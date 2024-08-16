#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

typedef struct s_free 
{
    void *ptr;
    struct s_free *next;
} t_free;

typedef struct s_list
{
    char *name;
    struct s_list *next;

} t_list;

t_free *is_free = NULL;

int ft_strlen(char *str)
{
    int i = 0;
    while(str[i])
        i++;
    return i;
}

void free_all()
{
    t_free *tmp;
    while(is_free)
    {
        tmp = is_free;
        is_free = is_free->next;
        free(tmp->ptr);
        free(tmp);
    }
    is_free = NULL;
}

t_free *create_free(void *ptr)
{
    t_free *new = malloc(sizeof(t_free));
    if(new == NULL)
        return NULL;
    new->ptr = ptr;
    new->next = NULL;
    return new;
}

void add_free(t_free **head, t_free *new)
{
    if(new == NULL)
        return;
    t_free *tmp = *head;
    if(*head == NULL)
    {
        *head = new;
    }
    else
    {
        while(tmp->next)
            tmp = tmp->next;
        tmp->next = new;
    }
}

int print_error(char *str)
{
    write(2, str, ft_strlen(str));
    return (1);
}

t_list *create_node(char *name)
{
    t_list *new = malloc(sizeof(t_list));
    if(new == NULL)
        return NULL;

    add_free(&is_free, create_free(new));
    new->name = name;
    new->next = NULL;
    return new;
}

void add_node(t_list **head, t_list *new)
{
    if(new == NULL)
        return;
    t_list *tmp = *head;
    if(*head == NULL)
    {
        *head = new;
    }
    else
    {
        while(tmp->next)
            tmp = tmp->next;
        tmp->next = new;
    }
}

int init_data(t_list **head, char **av)
{
    int i = 1;

    while(av[i])
        add_node(head, create_node(av[i++]));
    return 0;
}

int cd(char **cmd, int i)
{
    if(i != 2)
    {
        return print_error("error: cd: bad arguments\n"), 1;
    }

    if(chdir(cmd[1]) == -1)
    {
         return print_error("error: cd: cannot change directory to "), print_error(cmd[1]), print_error("\n"), 1;
    }
    return 0;
}

void set_pipe(int has_pipe, int *fd, int end)
{
	if (has_pipe && (dup2(fd[end], end) == -1 || close(fd[0]) == -1 || close(fd[1]) == -1))
		print_error ("error: fatal\n"), free_all(), exit(1);
}

int exec(t_list **head, char **env, char **cmd, int i)
{
    pid_t pid;
    int status;
    int fd[2];
    int is_pipe = 0;

    if (*head && strcmp((*head)->name, "|") == 0)
        is_pipe = 1;

    if (!is_pipe && strcmp(cmd[0], "cd") == 0)
        return cd(cmd, i);
    
    if(is_pipe && pipe(fd) == -1)
        print_error("error: fatal\n"), free_all(), exit(1);

    
    pid = fork();
    if (pid == -1)
         print_error("error: fatal\n"), free_all(),  exit(1);
    if(pid == 0)
    {
        set_pipe(is_pipe, fd, 1);
           if (!strcmp(cmd[0], "cd"))
            return cd(cmd, i);
        execve(cmd[0], cmd, env);
        print_error("error: cannot execute "), print_error(cmd[0]), print_error("\n"), exit(1);
    }
  
        waitpid(pid, &status, 0);
        set_pipe(is_pipe, fd, 0);
    return (0);    
}

char **conver_cmd(t_list **head, int i)
{
    char **cmd = malloc(sizeof(char *) * (i + 1));
    if(cmd == NULL)
        return NULL;
    int j = 0;
    while(j < i && *head)
    {
        cmd[j] = (*head)->name;
        *head = (*head)->next;
        j++;
    }
    cmd[j] = NULL;
    return cmd;
}



int execut_cmd(t_list **head, char **env)
{
    int i;
   
    while(*head)
    {
        i = 0;
        t_list *tmp = *head;;
        while(tmp && strcmp(tmp->name, "|") && strcmp(tmp->name, ";"))
        {
            tmp = tmp->next;
            i++;
        }
    
        if(i)
        {
            char **cmd = conver_cmd(head, i);
            if(cmd)
                exec(head, env, cmd, i);
            free(cmd);
        }
        if(*head == NULL)
            break;
        else
            *head = (*head)->next;
    }
    return 0;
}

int main (int ac, char **av, char **env)
{
    if(ac < 2)
        return 0;

    t_list *list = NULL;

    init_data(&list, av);
    execut_cmd(&list, env);
    free_all();
    return 0;
}

