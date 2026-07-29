# ════════════════════════════════════════════════════════════════════════════ #
#                           CONFIGURATION VARIABLES                            #
# ════════════════════════════════════════════════════════════════════════════ #

NAME		=	webserv

TEST_CFG	=	test_config_parser

CXX			=	c++
CXXFLAGS	=	-Wall -Wextra -Werror -std=c++98 -I$(INCDIR) -MMD -MP

INCDIR		=	include
SRCDIR		=	src
OBJDIR		=	obj

# ════════════════════════════════════════════════════════════════════════════ #
#                                 SOURCE FILES                                 #
# ════════════════════════════════════════════════════════════════════════════ #

CORE_SRCS	=	core/ServerManager.cpp

NET_SRCS	=	network/Server.cpp \
				network/Client.cpp

HTTP_SRCS	=	http/HttpRequest.cpp \
				http/HttpResponse.cpp

CONFIG_SRCS	=	config/ConfigParser.cpp \
				config/ServerConfig.cpp \
				config/LocationConfig.cpp

UTILS_SRCS	=	utils/Utils.cpp

SRCS		=	main.cpp \
				$(CORE_SRCS) \
				$(NET_SRCS) \
				$(HTTP_SRCS) \
				$(CONFIG_SRCS) \
				$(UTILS_SRCS)

# ════════════════════════════════════════════════════════════════════════════ #
#                                 OBJECT FILES                                 #
# ════════════════════════════════════════════════════════════════════════════ #

OBJS		= $(addprefix $(OBJDIR)/, $(SRCS:.cpp=.o))
DEPS		= $(OBJS:.o=.d)

# ════════════════════════════════════════════════════════════════════════════ #
#                                PHONY TARGETS                                 #
# ════════════════════════════════════════════════════════════════════════════ #

.PHONY: all clean fclean re \
        test test_config_parser test_multiplexer test_requests

# ════════════════════════════════════════════════════════════════════════════ #
#                                DEFAULT TARGET                                #
# ════════════════════════════════════════════════════════════════════════════ #

all: $(NAME)

# ════════════════════════════════════════════════════════════════════════════ #
#                                 BUILD RULES                                  #
# ════════════════════════════════════════════════════════════════════════════ #

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)
$(NAME): $(OBJS)
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $@
	@echo "✓ $(NAME) created successfully"

# ════════════════════════════════════════════════════════════════════════════ #
#                                CLEANUP RULES                                 #
# ════════════════════════════════════════════════════════════════════════════ #

clean:
	rm -rf obj

fclean: clean
	rm -f $(NAME) $(TEST_CFG)

re: fclean all

# ════════════════════════════════════════════════════════════════════════════ #
#                                  TEST RULES                                  #
# ════════════════════════════════════════════════════════════════════════════ #

TEST_SRCS	= tests/test_config_parser.cpp \
			  $(filter-out $(SRCDIR)/main.cpp, $(addprefix $(SRCDIR)/, $(SRCS)))

test: test_config_parser test_multiplexer test_requests

test_config_parser:
	@echo "Compiling $(TEST_CFG)..."
	@$(CXX) $(CXXFLAGS) $(TEST_SRCS) -o $(TEST_CFG)
	@echo "✓ $(TEST_CFG) created successfully"
	@chmod +x tests/run_parser_tests.sh
	@./tests/run_parser_tests.sh
	@rm -rf $(TEST_CFG) $(TEST_CFG).d $(TEST_CFG).dSYM

test_multiplexer:
	@python3 tests/test_multiplexer.py

test_requests:
	@python3 tests/test_requests.py
