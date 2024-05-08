CXX				= c++
CXXFLAGS		= -Wall -Wextra -Werror -MMD -MP -std=c++98

NAME			= ircserv

UTILS_DIR		= utils
UTILS_SRCS		= utils.cpp

ROOT_SRCS		= main.cpp \
					Server.cpp \
					Client.cpp \
					$(addprefix $(UTILS_DIR)/,$(UTILS_SRCS))

SRCS_DIR		= ./srcs
SRCS			= $(addprefix $(SRCS_DIR)/,$(ROOT_SRCS))

OBJS			= $(SRCS:.cpp=.o)

DEP				= $(patsubst %.cpp,%.d,$(SRCS))
-include $(DEP)

INCLUDE_FLAGS	= -I$(SRCS_DIR) \
					-I$(SRCS_DIR)/$(UTILS_DIR)

RM				= rm -f

.DEFAULT_GOAL	= all

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE_FLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(DEP)

fclean:
	$(RM) $(NAME) $(OBJS) $(DEP)

re: fclean
	make all

.PHONY: all bonus clean fclean re
