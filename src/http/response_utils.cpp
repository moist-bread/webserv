#include "../../inc/http/HTTP.hpp"

#include "../../inc/string_utils.tpp"

#include <algorithm>  // sort, max, min
#include <ctime>	  // localtime, strftime
#include <sys/stat.h> // stat
#include <dirent.h>	  // readdir, DIR

namespace response_utils
{

	bool is_error(t_status_code code) { return (code >= 400 ? true : false); }

	std::string backup_error_page(t_status_code status)
	{
		std::stringstream ss;
		ss << "<!DOCTYPE html>" << std::endl;
		ss << "<html lang=\"en\">" << std::endl;
		ss << "<head>" << std::endl;
		ss << "	<meta charset=\"UTF-8\">" << std::endl;
		ss << "	<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">" << std::endl;
		ss << "	<title>" << status << " " << HTTP::getReasonPhrase(status) << "</title>" << std::endl;
		ss << " <link rel=\"stylesheet\" href=\"/index.css\" type=\"text/css\">" << std::endl;
		ss << "</head>" << std::endl;
		ss << "<body>" << std::endl;
		ss << "<div class=\"text yellow-mark\"> ";
		ss << "Error: " << status << " " << HTTP::getReasonPhrase(status) << "</div>" << std::endl;
		ss << "</body>" << std::endl;
		ss << "</html>" << std::endl;
		return (ss.str());
	}

	std::string directory_listing(DIR *d, const std::string url, const std::string folder_str)
	{
		std::stringstream ss;
		ss << "<!DOCTYPE html>" << std::endl;
		ss << "<html lang=\"en\">" << std::endl;
		ss << "<head>" << std::endl;
		ss << "	<meta charset=\"UTF-8\">" << std::endl;
		ss << "	<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">" << std::endl;
		ss << "	<title>listing directory " << folder_str << "</title>" << std::endl;
		ss << " 	<link rel=\"stylesheet\" href=\"/index.css\" type=\"text/css\">" << std::endl;
		ss << "	</head>" << std::endl;
		ss << "	<body>" << std::endl;

		ss << "	<div id=\"wrapper\">" << std::endl;

		ss << "		<h1>";
		ss << "			<a href=\"" << url << "\">~</a> / ";
		std::string f_link = url;
		std::string f_name = folder_str;
		f_name.erase(0, 1);
		while (!f_name.empty())
		{
			size_t pin = f_name.find("/");
			if (pin == std::string::npos)
				pin = f_name.size();
			f_link += "/" + f_name.substr(0, pin);
			ss << "			<a href=\"" << f_link << "\">" << f_name.substr(0, pin) << "</a> / ";
			f_name.erase(0, pin + 1);
		}
		ss << "		</h1>" << std::endl;

		ss << "		<ul>" << std::endl;
		struct dirent *elem = readdir(d);
		while (elem != NULL)
		{
			if (elem->d_type != DT_DIR)
			{
				ss << "			<li>" << std::endl;
				ss << "				<a href=\"" << url << folder_str << "/" << elem->d_name << "\"" << std::endl;
				ss << "				title=\"" << elem->d_name << "\">" << std::endl;
				ss << "				<span>" << elem->d_name << "</span>" << std::endl;
				ss << "				</a>" << std::endl;
				ss << "			</li>" << std::endl;
			}
			elem = readdir(d);
		}
		ss << "		</ul>" << std::endl;

		ss << "	</div>" << std::endl;
		ss << "</body>" << std::endl;
		ss << "</html>" << std::endl;
		return (ss.str());
	}

#define RANGE_LIMIT 65536 // 64kb

	bool range_valid(int file_len, vector2 &ranges)
	{
		// Range syntax options
		// ->	<unit>=<range-start>-<range-end>
		// 		ex: 500-600 (from x to y)
		// ->	<unit>=<range-start>-
		// 		ex: 500- (from x to the end of file)
		// ->	<unit>=-<suffix-length>
		// 		ex: -600 (the last x bytes, from end - x to end of file)
		// ->	<unit>=<range-start>-<range-end>, …, <range-startN>-<range-endN>
		// 		ex: 500-600,700-800, 900-1000 (multiple ranges that cant overlap)
		// 			response works like a multipart form, using boundary strings
		//			and each section having it's own headers
		// 			Content-Type: multipart/byteranges; boundary=STRING

		// -- step 1: translate all syntaxes to work the same way

		for (vector2::iterator it = ranges.begin(); it != ranges.end(); it++)
		{
			if ((*it).first != VALUE_NOT_SET && (*it).second == VALUE_NOT_SET)
				(*it).second = file_len; // (from x to the end of file)
			else if ((*it).first == VALUE_NOT_SET && (*it).second != VALUE_NOT_SET)
			{
				(*it).first = file_len - (*it).second; // (the last x bytes, from end - x to end of file)
				(*it).second = file_len;
			}
			if ((*it).first >= (*it).second || (*it).second > static_cast<int>(file_len) || (*it).first < 0)
				return (false);
		}

		// -- step 2: copy map into a vector, sort and then loop to compare one to the next

		vector2 copy_range(ranges.begin(), ranges.end());
		size_t size_limit = copy_range.size() - 1;
		std::sort(copy_range.begin(), copy_range.end());

		for (size_t i = 0; i < size_limit; i++)
			if (std::max(copy_range[i].first, copy_range[i + 1].first) <= std::min(copy_range[i].second, copy_range[i + 1].second))
				return (false);

		// for (vector2::const_iterator it = ranges.begin(); it != ranges.end(); it++)
		// std::cout << CYN "    [" << (*it).first << "]" DEF " |" << (*it).second << "|" << std::endl;

		// -- step 3: force range limit size cap
		//			  (needs to be after cmp as to not cover up any request errors)
		for (vector2::iterator it = ranges.begin(); it != ranges.end(); it++)
			if ((*it).second - (*it).first > RANGE_LIMIT)
				(*it).second = (*it).first + RANGE_LIMIT;

		return (true);
	}

	std::string create_header_content_range(std::pair<int, int> range, t_status_code status, int size)
	{
		// Content-Range syntax:

		// unit =	"bytes"
		// range =	ranges[i].first"-"ranges[i].second OR * when RANGE_NOT_SATISFIABLE
		// size =	total file size OR * when unknown

		//	-> <unit> <range>/<size>	ex: bytes 0-1023/146515
		//	-> <unit> <range>/*			ex: bytes 67589/*
		//	-> <unit> */<size>			ex: bytes */67589

		std::string value = "bytes ";
		if (status == RANGE_NOT_SATISFIABLE)
			value += "*";
		else
			value += to_str(range.first) + "-" + to_str(range.second - 1);
		value += "/";
		if (size == VALUE_NOT_SET)
			value += "*";
		else
			value += to_str(size);
		return (value);
	}

	const char *define_content_type(std::string &extension)
	{
		// text types
		if (extension == "html")
			return ("text/html");
		else if (extension == "css")
			return ("text/css");
		else if (extension == "csv")
			return ("text/csv");
		else if (extension == "txt")
			return ("text/plain");

		// image types
		else if (extension == "jpeg" || extension == "jpg")
			return ("image/jpeg");
		else if (extension == "png")
			return ("image/png");
		else if (extension == "gif")
			return ("image/gif");
		else if (extension == "svg")
			return ("image/svg+xml");
		else if (extension == "webp")
			return ("image/webp");
		else if (extension == "ico")
			return ("image/vnd.microsoft.icon");

		// video and audio types
		else if (extension == "mp4")
			return ("video/mp4");
		else if (extension == "webm")
			return ("video/webm");
		else if (extension == "mpeg")
			return ("audio/mpeg");
		else if (extension == "wav")
			return ("audio/wav");

		// app types
		else if (extension == "pdf")
			return ("application/pdf");
		else if (extension == "form")
			return ("application/x-www-form-urlencoded");
		else if (extension == "json")
			return ("application/json");

		// multipart types
		else if (extension == "multiform")
			return ("multipart/form-data");
		else if (extension == "byteranges")
			return ("multipart/byteranges");

		else
			return ("application/octet-stream");
	}

	std::string random_name_generator(void)
	{
		std::string name;
		int length = 12 + (std::rand() % 10);

		for (int i = 0; i < length; i++)
		{
			const int type = std::rand() % 3;
			switch (type)
			{
			case 0: // number
				name += '0' + std::rand() % 10;
				break;
			case 1: // lower-case
				name += 'a' + std::rand() % 26;
				break;
			default: // upper-case
				name += 'A' + std::rand() % 26;
				break;
			}
		}
		return (name);
	}

	std::string date_format(time_t timestamp)
	{
		struct tm datetime = *std::localtime(&timestamp);
		char buff[30];
		std::string buffer;

		// example: Fri, 13 Mar 2026 17:32:50 GMT
		if (std::strftime(buff, sizeof buff, "%a, %d %b %Y %T GMT", &datetime))
			buffer = buff;
		return (buffer);
	}
}
