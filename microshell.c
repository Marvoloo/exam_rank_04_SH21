#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

typedef struct s_list t_list;

struct s_list
{
	int		type;
	char 	**argv;
	t_list	*next;
};

void	error_fatal()
{
	write(2, "error: fatal\n", 14);
	exit(1);
}

t_list *lstNew(int type)
{
	t_list *new;

	new = (t_list *)malloc(sizeof(t_list));
	if (new == 0)
		error_fatal();
	new->argv = 0;
	new->next = 0;
	new->type = type;
	return (new);	
}

t_list *lstLast(t_list *lst)
{
	while (lst->next)
		lst = lst->next;
	return (lst);
}

void	lstPushBack(t_list **lst, int size, int type, char **argv)
{
	t_list	*new;
	int 	i;

	if (*lst == 0)
	{
		*lst = lstNew(type);
		new = *lst;
	}
	else
	{
		new = lstNew(type);
		lstLast(*lst)->next = new;
	}
	if (type)
	{
		new->argv = (char **)malloc(sizeof(char *) * (size + 1));
		if (new->argv == 0)
			error_fatal();
		i = 0;
		while (i < size)
		{
			new->argv[i] = argv[i];
			i ++;
		}
		new->argv[i] = 0;	
	}	
}

void	lstClear(t_list **lst)
{
	t_list	*tmp;

	while ((*lst)->next)
	{
		tmp = (*lst)->next;
		free((*lst)->argv);
		free(*lst);
		(*lst) = tmp;
	}
	free((*lst)->argv);
	free(*lst);
}

t_list *parser(int argc, char **argv)
{
	int		i;
	int		begin;
	t_list	*lst;

	lst = 0;
	i = 1;
	while (i < argc)
	{
		if (strcmp(argv[i], "|") != 0 && strcmp(argv[i], ";") != 0)
		{
			begin = i;
			while (i < argc && strcmp(argv[i], "|") != 0 && strcmp(argv[i], ";") != 0)
				i ++;
			lstPushBack(&lst, i - begin, 1, argv + begin);
			continue ;	
		}
		if (strcmp(argv[i], "|") == 0)
			lstPushBack(&lst, i, 0, 0);
		i ++;	
	}
	return (lst);
}

void	executor(t_list *lst, char **envp)
{
	int	pipefd[2];
	int	pid;
	int	STD_IN;

	STD_IN = dup(0);
	while (lst)
	{
		if (lst->type == 1)
		{
			if (strcmp(lst->argv[0], "cd") == 0)
			{
				if (lst->argv[1] == 0 || lst->argv[2] != 0)
				write(2, "error: cd: bad arguments\n", 26);
				else if (chdir(lst->argv[1]) < 0)
				{
					write(2, "error: cd: cannot change directory to ", 39);
					write(2 , lst->argv[1], strlen(lst->argv[1]));
					write(2, "\n", 1); 
				}
			}
			else 
			{
				if (pipe(pipefd) < 0)
					error_fatal();
				pid = fork();
				if (pid < 0)
					error_fatal();
				if (pid == 0)
				{
					close(pipefd[0]);
					if (lst->next && lst->next->type == 0)
						dup2(pipefd[1], 1);
					else
					{
						close(pipefd[1]);
					}
					if (execve(lst->argv[0], lst->argv, envp))
					{
						write(2, "error: cannot execute " , 22);
						write(2, lst->argv[0], strlen(lst->argv[0]));
						write(2, "\n", 1);
						exit(1);
					}
				}
				waitpid(pid, 0, 0);
			}
		}
		if (lst->next && lst->next->type == 0)
		{
			dup2(pipefd[0], 0);
			close(pipefd[1]);
			close(pipefd[0]);
			lst = lst->next;
		}
		else
		{
			close(pipefd[0]);
			close(pipefd[1]);
			dup2(STD_IN, 0);
		}
		lst = lst->next;
	}
	close(STD_IN);
}

int main(int argc, char **argv, char **envp)
{
	t_list *lst = 0;
	if (argc < 2)
		error_fatal();
	lst = parser(argc, argv);
	if (lst)
	{
		executor(lst, envp);
		lstClear(&lst);
	}
}