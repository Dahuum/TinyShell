#include "minishell.h"

t_sig g_sig;


void history(char *line)
{
    if (line && *line)
        add_history(line);
}

char *initialise_prompt(void)
{
    char *prompt;
    char dir[200];
    char p[200];
    char *pwd;
    char hostname[1024];

    gethostname(hostname, 1024);

    pwd = getcwd(dir, 4096);
    while (*pwd)
    {
        if (*pwd == '/')
        {
            pwd++;
            _strcpy(p, pwd);
        }
        pwd++;
    }
    prompt = _strcat(getenv("USER"), "@");
    prompt = _strcat(prompt, hostname);
    prompt = _strcat(prompt, ":");
    prompt = _strcat(prompt, p);
    prompt = _strcat(prompt, "$ ");
    return (prompt);
}

void redir_exec(t_tiny *tiny, t_token *token)
{
    t_token *prev;
    t_token *next;
    int pipe;

    prev = prev_sep(token);
    next = next_sep(token);
    pipe = 0;
    if (is_type(prev, TRUNC))
        redir(tiny, token, TRUNC);
    else if (is_type(prev, APPEND))
        redir(tiny, token, APPEND);
    else if (is_type(prev, APPEND_OPPOSITE))
        redir_her_doc(tiny, token);
    else if (is_type(prev, INPUT))
        input(tiny, token);
    else if (is_type(prev, PIPE))
        pipe = tinypipe(tiny);
    if (next && is_type(next, END) == 0 && pipe != 1)
        redir_exec(tiny, next->next);
    if ((is_type(prev, END) || is_type(prev, PIPE) ||!prev)
            && tiny->no_exec == 0 && pipe != 1)
            exec_cmd(tiny, token);

}


void exec(t_tiny *tiny)
{
    t_token *token;
    int status;

    token = next_run(tiny->start);
    if (is_types(tiny->start, "TAIW"))
        token = tiny->start->next; 
    while (tiny->exit == 0 && token)
    {
        tiny->parent = 1;
        tiny->charge = 1;
        tiny->last = 1;
        redir_exec(tiny, token);
        reset_std(tiny);
        _close_fds(tiny);
        reset_fds(tiny);
        waitpid(-1, &status, 0);
		status = WEXITSTATUS(status);
        tiny->no_exec = 0;
        token = token->next;
        token = next_run(token);
    }
}

int main(int ac, char **av, char **env)
{
    if(ac != 1 || env == NULL || *env == NULL)
        return (printsdr("Error: No environment found. Exiting..."));
    t_tiny tiny;
    
    (void)ac;
    (void)av;
    tiny.in = dup(STDIN);
    tiny.out = dup(STDOUT);
    tiny.exit = 0;
    tiny.ret = 0;
    tiny.no_exec = 0;
    reset_fds(&tiny);
    init_env(&tiny, env);
    private_init_env(&tiny,env);
    set_shell_level(tiny.env);
    while (tiny.exit == 0)
    {
        char *read = initialise_prompt();
        disable_echo();
        signal(SIGINT, int_handler);
        signal(SIGQUIT, SIG_IGN);
        tiny.line = readline(read);
        if (!tiny.line || _strlen(tiny.line) == 0)
            continue;
        history(tiny.line);
        if (tiny.line && !check_syntax(tiny.line))
        {
            parse(&tiny);
            exec(&tiny);
        }

    }
    // _memdel(read);
    free_all(&tiny);
    rl_clear_history();
}
