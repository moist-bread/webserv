# =====>┊( NAMES )┊
NAME	=	webserv

# =====>┊( CMDS AND FLAGS )┊
CXX		 =	c++
CXXFLAGS =	-Wall -Wextra -Werror -g -std=c++98
VAL		 =	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --track-fds=yes -s

# =====>┊( DIRECTORIES )┊
SRC_DIR		=	src
OBJ_DIR		=	obj
SOCK_DIR	=	sockets
PARSE_DIR	=	parse
OTHER_DIR	= 	other

# =====>┊( SRC/OBJS )┊
MAIN_SOCK			=	main_sock.cpp
MAIN_PARSE			=	main_parse.cpp

SOCK_FILES_C	=	SocketController.cpp CgiHandler.cpp ListeningSocket.cpp ConnectingSocket.cpp \
					BindingSocket.cpp ASimpleServer.cpp Server.cpp FileDescriptor.cpp Client.cpp

PARSE_FILES_C	=

OTHER_FILES_C	=	Class.cpp signal.cpp Request.cpp

OBJS_MAIN_SOCK	=	$(addprefix $(OBJ_DIR)/, $(MAIN_SOCK:.cpp=.o))
OBJS_MAIN_PARSE	=	$(addprefix $(OBJ_DIR)/, $(MAIN_PARSE:.cpp=.o))

OBJS_SOCK		=	$(addprefix $(OBJ_DIR)/, $(SOCK_FILES_C:.cpp=.o))
OBJS_PARSE		=	$(addprefix $(OBJ_DIR)/, $(PARSE_FILES_C:.cpp=.o))
OBJS_OTHER		=	$(addprefix $(OBJ_DIR)/, $(OTHER_FILES_C:.cpp=.o))


# =====>┊( COMP RULES )┊
all: $(NAME)

$(NAME): $(OBJS_MAIN_SOCK) $(OBJS_SOCK) $(OBJS_PARSE) $(OBJS_OTHER)
	$(M_COMOBJS)
	@$(CXX) $(CXXFLAGS) $(OBJS_MAIN_SOCK) $(OBJS_SOCK) $(OBJS_PARSE) $(OBJS_OTHER) -o $(NAME)
	$(M_COM)


parse: fclean $(OBJS_MAIN_PARSE) $(OBJS_SOCK) $(OBJS_PARSE) $(OBJS_OTHER)
	$(M_COMOBJS)
	@$(CXX) $(CXXFLAGS) $(OBJS_MAIN_PARSE) $(OBJS_SOCK) $(OBJS_PARSE) $(OBJS_OTHER) -o $(NAME)
	$(M_COM)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/$(SOCK_DIR)/%.cpp | $(OBJ_DIR)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/$(PARSE_DIR)/%.cpp | $(OBJ_DIR)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/$(OTHER_DIR)/%.cpp | $(OBJ_DIR)
	@$(CXX) $(CXXFLAGS) -c $< -o $@


$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)


# =====>┊( EXEC RULES )┊
exe: all
	./$(NAME) configfile

rexe: re
	@echo "\n"
	./$(NAME) configfile

val: all
	$(M_VAL)
	$(VAL) ./$(NAME) configfile

# =====>┊( STANDARD RULES )┊
clean:
	$(M_REMOBJS)
	@rm -rf $(OBJ_DIR)

fclean: clean
	$(M_REM)
	@rm -rf $(NAME)

re:	fclean all
	$(M_RE)

.PHONY: all clean fclean re rexe


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
