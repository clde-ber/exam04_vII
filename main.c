#include <sys/wait.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>

static int pipe_end = 0;

int ft_strlen(char *str)
{
    int i = 0;

    while(str[i])
        i++;
    return i;
}

void ft_putstr_fd(int fd, char *str)
{
    write(fd, str, ft_strlen(str));
}

int ft_strcmp(char *s1, char *s2)
{
    int i = 0;

    while (s1[i] && s2[i] && s1[i] == s2[i])
        i++;
    return s1[i] - s2[i];
}

char *ft_strdup(char *str)
{
    int i = 0;
    char *res= NULL;

    if (!(res = malloc(sizeof(char) * (ft_strlen(str) + 1))))
        return NULL;
    while (str[i])
    {
        res[i] = str[i];
        i++;
    }
    res[i] = '\0';
    return res;
}

void free_double_tab(char **tab)
{
    int i = 0;

    while (tab[i])
    {
        free(tab[i]);
        tab[i] = NULL;
        i++;
    }
    free(tab);
    tab = NULL;
}

void free_triple_tab(char ***tab)
{
    int i = 0;
    while (tab[i])
    {
        free_double_tab(tab[i]);
        i++;
    }
    free(tab);
    tab = NULL;
}

void free_double_tab_len(int **tab, int len)
{
    int i = 0;

    while (i < len)
    {
        free(tab[i]);
        tab[i] = NULL;
        i++;
    }
    free(tab);
    tab = NULL;
}

int double_tab_len(char **tab)
{
    int i = 0;

    while (tab[i])
        i++;
    return i;
}

int triple_tab_len(char ***tab)
{
    int i = 0;

    while (tab[i])
        i++;
    return i;
}

int count_elements(char **tab, char *delimiter)
{
    int i = 0;
    int count = 0;

    while (tab[i])
    {
        if (ft_strcmp(tab[i], delimiter) == 0)
            count++;
        i++;
    }
    return ++count;
}

int do_cd(char **cmd)
{
    if (cmd[1] && (chdir(cmd[1]) == -1))
    {
        ft_putstr_fd(2, "error: cd: cannot change directory to ");
        ft_putstr_fd(2, cmd[1]);
        ft_putstr_fd(2, "\n");
        return 1;
    }
    if (cmd[1] && cmd[2] && ft_strcmp(cmd[2], ";"))
    {
        ft_putstr_fd(2, "error: cd: bad arguments\n");
        return 1;
    }
    return 0;
}

int exec(char *cmd, char **cmd_list, char **env, int has_pipe, int **pipefd, int i)
{
    int ret = 1;
    pid_t pid = 0;
    int status = 0;

    if ((pid = fork()) == -1)
    {
        ft_putstr_fd(2, "error: fatal\n");
        return 1;
    }
    if (pid == 0)
    {
        if (has_pipe)
        {
            close(pipefd[i - 1][1]);
            dup2(pipefd[i - 1][0], STDIN_FILENO);
            if (!pipe_end)
            {
                close(pipefd[i][0]);
                dup2(pipefd[i][1], STDOUT_FILENO);
            }
        }
        if ((ret = execve(cmd,cmd_list, env)) == -1)
        {
            ft_putstr_fd(2, "error: cannot execute ");
            ft_putstr_fd(2, cmd);
            ft_putstr_fd(2, "\n");
            exit(status);
        }
        exit(status);
    }
    else
    {
        if (has_pipe)
        {
            close(pipefd[i - 1][1]);
            close(pipefd[i - 1][0]);
        }
        waitpid(pid, &status, 0);
        if (has_pipe && pipe_end)
        {
            close(pipefd[i][0]);
            close(pipefd[i][1]);
            close(pipefd[0][0]);
            close(pipefd[0][1]);
        }
        if (WIFEXITED(status))
            ret = WIFEXITED(status);
    }
    return ret;
}

int exec_pipes(char ***cmd, char **env, int len)
{
    int i = 0;
    int **pipefd = NULL;
    int ret = 0;

    if (!(pipefd = malloc(sizeof(int *) * len)))
        return 1;
    for (int x = 0; x < len; x++)
    {
        if (!(pipefd[x] = malloc(sizeof(int) * 2)))
        {
            free_double_tab_len(pipefd, x);
            return 1;
        }
    }
    pipe_end = 0;
    while (cmd[i])
    {
        if (i == 0)
        {
            pipe(pipefd[i]);
            pipe(pipefd[i + 1]);
        }
        else
        {
            if (!cmd[i + 1])
                pipe_end = 1;
            pipe(pipefd[i + 1]);
        }
        ret = exec(cmd[i][0], cmd[i], env, 1, pipefd, i + 1);
        i++;
    }
    free_double_tab_len(pipefd, len);
    return ret;
}

char **create_subtab(char **cmd, int start, int end)
{
    int i = 0;
    char **res = NULL;
    if (!(res = malloc(sizeof(char *) * (end - start + 1))))
        return 0;
    while(start < end)
        res[i++] = ft_strdup(cmd[start++]);
    res[i] = NULL;
    return res;
}

char ***parse_on_delimiter(int start ,char **cmd, char *delimiter)
{
    int i = start;
    int x = 0;
    char ***res = NULL;

    if (!(res = malloc(sizeof(char **) * (count_elements(cmd, delimiter) + 1))))
        return NULL;
    while(cmd[i])
    {
        if (ft_strcmp(cmd[i], delimiter) == 0)
        {
            res[x] = create_subtab(cmd, start, i);
            while (cmd[i] && (ft_strcmp(cmd[i], delimiter) == 0))
                i++;
            if (!cmd[i])
            {
                res[++x] = NULL;
                return res;
            }
            start = i;
            x++;
        }
        i++;
    }
    res[x] = create_subtab(cmd, start, i);
    res[++x] = NULL;
    return res;
}

int main(int ac, char **av, char **env)
{
    int i = 0;
    int ret = 0;
    char ***cmd = NULL;
    char ***pipe_cmd = NULL;

    if (ac < 2)
        return 1;
    if (!(cmd = parse_on_delimiter(1, av, ";")))
        return 1;
    while (cmd[i] && cmd[i][0])
    {
        pipe_cmd = parse_on_delimiter(0, cmd[i], "|");
        if (pipe_cmd && pipe_cmd[0] && pipe_cmd[0][0] && double_tab_len(cmd[i]) != double_tab_len(pipe_cmd[0]) && ft_strcmp(pipe_cmd[0][0], "cd"))
            ret = exec_pipes(pipe_cmd, env, count_elements(cmd[i],"|") + 1);
        else if (ft_strcmp(cmd[i][0], "cd"))
            ret = exec(cmd[i][0], cmd[i], env, 0, NULL, 0);
        else
            ret = do_cd(cmd[i]);
        free_triple_tab(pipe_cmd);
        i++;
    }
    free_triple_tab(cmd);
    return ret;
}
