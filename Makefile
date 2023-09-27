NAME = mini_serv
CC = clang
CFLAGS  = -Wall -Werror -Wextra -fsanitize=address -g3
FILES = mini_serv 

SRCS = $(addsuffix .c,$(FILES))
OBJS = $(addsuffix .o, $(FILES))

all: $(NAME)

.c.o: $(SRCS)
	$(CC) $(CFLAGS) -c $< -o $@

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

clean: 
	rm -rf $(OBJS)

fclean: clean
	rm -rf $(NAME)

re: fclean all

.PHONY: all clean fclean re