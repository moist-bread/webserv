# =====>┊( NAMES )┊
NAME	=	webserv

# =====>┊( CMDS AND FLAGS )┊
DEBUG	 =	1
CXX		 =	c++
CXXFLAGS =	-Wall -Wextra -Werror -g -std=c++98 -D DEBUG=$(DEBUG)
VAL		 =	valgrind --leak-check=full --trace-children=yes --show-leak-kinds=all --track-origins=yes --track-fds=yes -s

# =====>┊( DIRECTORIES )┊
SRC_DIR		=	src
OBJ_DIR		=	obj
SOCK_DIR	=	sockets
CONF_DIR	=	serverConfig
OTHER_DIR	= 	other

# =====>┊( SRC/OBJS )┊
MAIN			=	main.cpp

SOCK_FILES_C	=	SocketController.cpp CgiHandler.cpp ListeningSocket.cpp ConnectingSocket.cpp \
					BindingSocket.cpp ASimpleServer.cpp Server.cpp FileDescriptor.cpp Client.cpp

CONF_FILES_C	= 	Lexer.cpp ConfigParser.cpp Config.cpp ServerConfig.cpp LocationConfig.cpp TokenStream.cpp\

OTHER_FILES_C	=	signal.cpp Request.cpp Response.cpp HTTP.cpp response_utils.cpp Inspect.cpp

OBJS_MAIN	=	$(addprefix $(OBJ_DIR)/, $(MAIN:.cpp=.o))

OBJS_SOCK		=	$(addprefix $(OBJ_DIR)/, $(SOCK_FILES_C:.cpp=.o))
OBJS_CONF		=	$(addprefix $(OBJ_DIR)/, $(CONF_FILES_C:.cpp=.o))
OBJS_OTHER		=	$(addprefix $(OBJ_DIR)/, $(OTHER_FILES_C:.cpp=.o))


# =====>┊( COMP RULES )┊
all: $(NAME)

$(NAME): $(OBJS_MAIN) $(OBJS_SOCK) $(OBJS_CONF) $(OBJS_OTHER)
	$(M_COMOBJS)
	@$(CXX) $(CXXFLAGS) $(OBJS_MAIN) $(OBJS_SOCK) $(OBJS_CONF) $(OBJS_OTHER) -o $(NAME)
	$(M_COM)

reparse: fclean parse 

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/$(SOCK_DIR)/%.cpp | $(OBJ_DIR)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/$(CONF_DIR)/%.cpp | $(OBJ_DIR)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/$(OTHER_DIR)/%.cpp | $(OBJ_DIR)
	@$(CXX) $(CXXFLAGS) -c $< -o $@


$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)


# =====>┊( EXEC RULES )┊
exe: all
	./$(NAME) config_files/nginx.conf

rexe: re
	@echo "\n"
	./$(NAME) config_files/nginx.conf

val: all
	$(M_VAL)
	$(VAL) ./$(NAME) config_files/nginx.conf

main_test:
	@$(CXX) $(CXXFLAGS) src/main_sock.cpp -o $(NAME)
	./$(NAME) config_files/nginx.conf

debug: fclean
	make all DEBUG=0
	./$(NAME) config_files/nginx.conf

test: all
	./$(NAME) config_files/tester.conf

# =====>┊( STANDARD RULES )┊
clean:
	$(M_REMOBJS)
	@rm -rf $(OBJ_DIR)

fclean: clean
	$(M_REM)
	@rm -rf $(NAME)

re:	fclean all
	$(M_RE)

.PHONY: all clean fclean re rexe val main_test debug


# =====>┊( COSMETICS )┊

# ==┊ normal colors
DEF	=	\e[0;39m
BLK	=	\e[0;30m
BLU	=	\e[0;34m
MAG =	\e[0;35m
GRN	=	\e[0;32m
YEL	=	\e[0;33m

# ==┊ bold colors
BCYN	=	\e[1;36m
BBLU	=	\e[1;34m

# ==┊ background colors
CYNB	=	\e[46m
YELB	=	\e[43m
BLUB	=	\e[44m

# ==┊ messages
M_COMOBJS	= @echo "$(BLK)-->┊$(GRN)  Compiling: $(BBLU)$(NAME)/objs $(BLK)$(DEF)"
M_COM		= @echo "$(BLK)-->┊$(GRN)  Compiled:  $(DEF)$(BLUB) $(NAME) $(BLK)$(DEF) ✅\n"
M_REMOBJS	= @echo "$(BLK)-->┊$(BLU)  Removing: $(BBLU) $(NAME)/objs $(BLK)$(DEF)"
M_REM		= @echo "$(BLK)-->┊$(BLU)  Removed:   $(DEF)$(BLUB) $(NAME) $(BLK)$(DEF) ✅\n"
M_RE		= @echo "$(BLK)... $(BLU)  Re Done   $(DEF)$(BCYN) $(NAME) is ready !!$(DEF)"
M_VAL		= @echo "$(BLK)... $(YEL)  Running with Valgrind 🧠$(DEF)"
