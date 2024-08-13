#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int nbr_cmd  = 0;

typedef struct s_address
{
    void *content;
    struct s_address *next;
}t_address;
t_address *g_global;

typedef struct s_args
{
 char *content;
 struct s_args *next;
}t_args;

typedef struct s_list 
{
    t_args *args;
    int is_break;
    int is_pipe;
    int     in;
    int     out;
    struct s_list *next;
}t_list;


typedef struct s_mini
{
    int     **fd_arr;
    char    **cmd;
    int     last_pid;
    t_list *head;
}t_mini;
void free_address(t_address **head)
{
    t_address *temp = NULL;
    while(*head)
    {
        temp = *head;
        *head = (*head)->next;
        free(temp->content);
        free(temp);
    }
    *head = NULL;
}
t_address *new_free(void *content)
{
    t_address *node = malloc(sizeof(t_address));
    node->content = content;
    node->next  = NULL;
    return node;
}
void add_free(t_address **head, t_address *new)
{
    if(!*head)
    {
        *head = new;
        return ;
    }
    else
    {
        t_address *temp = *head;
        while(temp->next)
        temp = temp->next;
        temp->next = new;
    }
}

t_list *new_cm(t_args *new_args, int is_break, int is_pipe)
{
    t_list *node = malloc(sizeof(t_list));
    if(!node)
    return NULL;
    add_free(&g_global, new_free(node));
    node->args = new_args;
    node->in = 0;
    node->out = 1;
    node->is_break = is_break;
    node->is_pipe = is_pipe;
    node->next = NULL;
    return node;
}
t_args *new_arg(char *content)
{
    t_args *node = malloc(sizeof(t_args));
    if(!node)
     return NULL;
 add_free(&g_global, new_free(node));

    node->content = content;
    node->next = NULL;
     return node;
}
void add_args(t_args **head, t_args *new)
{
    if(!*head || !head)
    {
        *head = new;
        return ;
    }
    else
    {
        t_args *temp = *head;
        while(temp->next)
         temp = temp->next;
        temp->next = new;
    }
}
void add_cm(t_list **head, t_list *new)
{
    if(*head == NULL || head == NULL)
     {
        *head = new;
        return ;
     }
     else
     {
        t_list *temp = *head;
         while(temp->next)
            temp = temp->next;
     temp->next = new; 
     }
}

int ft_strlen(char *str)
{
    int i = 0;
    while(str[i])
         i++;
    return i;
}
int err(char *str)
{
    while (*str)
        write(2, str++, 1);
    return 1;
}
int parse(t_list **prog, char **av)
{
    int i = 1;
    t_args *args_cmd = NULL;
    t_args *is_break = new_arg(";");
    t_args *is_pipe = new_arg("|");
    t_list *cmd = NULL;

    // if (!av[1] || !strcmp(av[1], "|"))
    //    return 1;

    while (av[i])
    {
        // if ((!strcmp(av[i], "|")) && 
        //     (!av[i + 1] || !strcmp(av[i + 1], ";") || !strcmp(av[i + 1], "|")))
        //     exit(1);

        if(av[i] && strcmp(av[i], "|") != 0 && strcmp(av[i], ";") != 0)
            add_args(&args_cmd, new_arg(av[i]));
        if (strcmp(av[i], "|") == 0)
        {
            if(args_cmd)
            {
              add_cm(&cmd, new_cm(args_cmd, 0, 0));
              add_cm(&cmd, new_cm(is_pipe, 0, 1));
             args_cmd = NULL;
            }
            else
             add_cm(&cmd, new_cm(is_pipe, 0, 1));
        }
        if (strcmp(av[i], ";") == 0)
        {
            if(args_cmd)
            {
              add_cm(&cmd, new_cm(args_cmd, 0, 0));
              add_cm(&cmd, new_cm(is_break, 1, 0));
             args_cmd = NULL;
            }
            else
             add_cm(&cmd, new_cm(is_break, 1, 0));

        }
        i++;
    }
    if (args_cmd)
        add_cm(&cmd, new_cm(args_cmd, 0, 0));

    *prog = cmd;      
    return 0;
}

void close_pipe(t_mini *prog)
{
    if (prog->fd_arr == NULL)
        return;
    int **fd = prog->fd_arr;
    int i = 0;
    while (fd[i])
    {
        close(fd[i][0]);
        close(fd[i][1]);
        i++;
    }
}


void free_pipe(t_mini *prog)
{
    if(prog->fd_arr == NULL)
     return ;
    int **fd = prog->fd_arr;
    int  i  = 0;
  while(fd[i])
  {
    free(fd[i]);
    i++;
  }
  free(fd);
  prog->fd_arr  = NULL;
}

void open_pipe(t_mini *prog, t_list *cmd)
{
    int count = 0;
    int i = 0;
    t_list *temp = cmd;
    while(temp)
    {
        if(temp->is_break == 1)
          break;
        if(temp->is_pipe == 1)
           {
            temp = temp->next;
            continue;
           }
       count++;
        temp = temp->next;
    }
    if(count == 1 || count == 0)
     return ;
    nbr_cmd = count;
    prog->fd_arr = malloc(sizeof(int *) * count);
    if(!prog->fd_arr)
    {
       return ;
    }
    while(i < count - 1)
    {
        prog->fd_arr[i] = malloc(sizeof(int) * 2);
        if(!prog->fd_arr[i])
        {
           
            return ;
        }
        if(pipe(prog->fd_arr[i++]) < 0)
        {
            err("error: fatal\n");
            exit(1);
        }
    }
  prog->fd_arr[i] = NULL;
}
void set_pipe(t_mini *prog, t_list *cmd)
{
    t_list *temp = cmd;
    int i = 1;
    
    if(prog->fd_arr == NULL)
        return ;
    int **fd = prog->fd_arr;
    temp->out = fd[0][1];
    temp = temp->next;
    while(fd[i])
    {
        temp->in = fd[i - 1][0];
        temp->out = fd[i][1];
        i++;
        temp = temp->next;
    }
    temp->in = fd[i - 1][0];
}
char **convert_cmd(t_args *head)
{
    int i = 0;
    int count = 0;
    if(head == NULL)
     return NULL;
    t_args *temp =  head;
    t_args *cur  = head;
     while(temp)
    {
          count++;
        temp = temp->next;
    }
    char **cmd = malloc(sizeof(char *) * (count + 1));
     add_free(&g_global, new_free(cmd));
   while(cur)
   {
    cmd[i] = cur->content;
    cur = cur->next;
    i++;
   }
   cmd[i] = NULL;
   return cmd;  
}
int exec(t_list *head, t_mini *prog, char **env)
{
    pid_t pid;
    pid = fork();
    if(!pid)
    {
        if(dup2(head->in, 0) < 0)
        return (err("error: fatal\n"));
        if(dup2(head->out, 1) < 0)
         return (err("error: fatal\n"));
        close_pipe(prog);
        free_pipe(prog);
        if(execve(prog->cmd[0], prog->cmd, env) == -1)
        {
            err("error: cannot execute ");
            err(prog->cmd[0]);
            err("\n");
            exit(127);
        }
    }
    if(head->next == NULL)
       prog->last_pid = pid;

    return 0;
}

int cd(t_list *cmd)
{
    t_args *tmp = cmd->args;
    t_args *cur = cmd->args;
    int count =  0;
    while(tmp)
    {
        count++;
        tmp = tmp->next;
    }
 if(count != 2)
  return (err( "error: cd: bad arguments\n"));
 cur = cur->next;
 if(chdir(cur->content) < 0)
 return (err("error: cd: cannot change directory to path_to_change\n"));

 return 0;
}
int execut(t_mini *prog, char **env)
{
    t_list *temp = prog->head;
    int status = 0;
    if(!temp)
     return 0;
    
    if(temp->is_break == 1 && temp->next == NULL)
     return 0;
    if(temp->is_break == 1 && (temp->next != NULL || temp->is_pipe == 1))
    {
        temp = temp->next;
        nbr_cmd = 0;
    }
    if(temp->is_pipe == 1 && (temp->next == NULL || temp->next->is_break == 1 || temp->next->is_pipe == 1))
        return 0;
    open_pipe(prog, temp);
    set_pipe(prog, temp);
    while(temp)
    {
        if((temp->is_pipe == 1 || temp->is_break == 1) && ( temp->next == NULL || temp->next->is_break == 1 
        || temp->next->is_pipe == 1))
        {
            free_pipe(prog);
            close_pipe(prog);
            prog->fd_arr = NULL;
            free_address(&g_global);
         exit(1);
        }
        else if (temp->is_pipe == 1)
        {
            temp = temp->next;
            nbr_cmd = 0;
        }
        if(temp->is_break == 1)
        {
            temp = temp->next;
            nbr_cmd = 0;
        close_pipe(prog);
        free_pipe(prog);
        open_pipe(prog, temp);
        set_pipe(prog, temp);
        }
      prog->cmd = convert_cmd(temp->args);
      if(strcmp(prog->cmd[0], "cd") == 0)
       return (cd(temp));
      exec(temp, prog, env);
    temp = temp->next;
    }
    close_pipe(prog);
    free_pipe(prog);
if (prog->last_pid == 0)
		return 0;
 waitpid(prog->last_pid, &status, 0);
 if(WIFEXITED(status))
   status = WEXITSTATUS(status); 
  while(wait(NULL) > 0)
  ;
  return status;
}

int main (int ac, char **av, char **env)
{
    t_mini prog;
    prog.head = NULL;
    prog.fd_arr = NULL;
    prog.last_pid = 0;
    prog.cmd = NULL;
    int res = 0;
    
    
    if(ac > 1)
    {
        g_global = malloc(sizeof(t_address));
        g_global->content = NULL;
        g_global->next = NULL;
       parse(&prog.head, av);
      res = execut(&prog, env);
      free_address(&g_global);
    }
return res;
}

