#include "shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>   // for open()
#include <sys/prctl.h>  // for setting process name
#include <time.h>       // for time(), localtime(), strftime()
#include <limits.h>     // for PATH_MAX
#include <sys/time.h>     // for struct timeval
#include <sys/resource.h> // for struct rusage and getrusage()
#include <sys/wait.h>     // for waitpid()
#include <unistd.h>       // for fork(), exec*, etc.
#include <stdio.h>        // for printf()
#include <ctype.h>
#include <termios.h> // to detect up down button inputs

void type_prompt(void);

#define MAX_ARGS 64
#define MAX_RC_LINES 100
#define MAX_LINE_LEN 1024

/*history list*/
#define MAX_HISTORY 100
static char *history[MAX_HISTORY];
static int  history_count = 0;


//global snapshot variables
static struct rusage prev_usage = {0};
static struct rusage curr_usage;

//struct for customized appearance
typedef struct {
  char *prompt_format;    // e.g. "\\u@\\h:\\w$ "
  char *color_scheme;     // e.g. "default", "dark", "light", "solarized"
  int   show_timestamp;   // 0 or 1
} shell_config_t;

static shell_config_t config;

static const char *calc_p;

// forward decls
static double parse_expr(void);
static double parse_term(void);
static double parse_factor(void);

// expr := term { (+|-) term }
static double parse_expr(void) {
  double v = parse_term();
  while (*calc_p == '+' || *calc_p == '-') {
    char op = *calc_p++;
    double r = parse_term();
    v = (op == '+') ? v + r : v - r;
  }
  return v;
}

// term := factor { (*|/) factor }
static double parse_term(void) {
  double v = parse_factor();
  while (*calc_p == '*' || *calc_p == '/') {
    char op = *calc_p++;
    double r = parse_factor();
    v = (op == '*') ? v * r : v / r;
  }
  return v;
}

// factor := number | '(' expr ')'
static double parse_factor(void) {
  while (isspace(*calc_p)) calc_p++;
  if (*calc_p == '(') {
    calc_p++;               // skip '('
    double v = parse_expr();
    if (*calc_p == ')') calc_p++;
    return v;
  }
  char *end;
  double v = strtod(calc_p, &end);
  calc_p = end;
  return v;
}

// Draw the prompt according to current config
void draw_prompt() {
    char buf[128], cwd[2048], host[2048];
    char *p = config.prompt_format;

    // optional timestamp
    if (config.show_timestamp) {
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);
        strftime(buf, sizeof(buf), "[%H:%M:%S] ", tm);
        printf("%s", buf);
    }

    // color logic
    if (strcmp(config.color_scheme, "solarized")==0)
        printf("\x1b[33m");
    else if (strcmp(config.color_scheme, "dark")==0) {
    	// bright white
    	printf("\x1b[37m");   
    } else if (strcmp(config.color_scheme, "light")==0) {
    	// black text on white background
    	printf("\x1b[30;47m");
    } else if (strcmp(config.color_scheme, "blue")==0) {
    	// blue text
    	printf("\x1b[34m");
    } else if (strcmp(config.color_scheme, "green")==0) {
    	// green text
    	printf("\x1b[32m");
    } else if (strcmp(config.color_scheme, "red")==0) {
    	// red text
    	printf("\x1b[31m");
    } else if (strcmp(config.color_scheme, "default")==0) {
	//default text
	printf("\x1b[0m");
    }

    // new parsing loop  
    while (*p) {
        if (p[0]=='\\' && p[1]) {
            switch (p[1]) {
                case 'u': 
                    printf("%s", getlogin());  
                    break;
                case 'h':
                    gethostname(host, sizeof(host));
                    printf("%s", host);
                    break;
                case 'w':
                    getcwd(cwd, sizeof(cwd));
                    printf("%s", cwd);
                    break;
                case '\\': 
                    putchar('\\'); 
                    break;
                case '$':  
                    putchar('$'); 
                    break;
                default:
                    // unknown escape: just print it literally
                    putchar(p[1]);
            }
            p += 2;
        } else {
            putchar(*p++);
        }
    }

    // reset color
    if (strcmp(config.color_scheme, "default")!=0)
        printf("\x1b[0m");

    fflush(stdout);
}

char output_file_path[2048];
int already_ran_rc = 0;

void process_rc_file() {
    if (already_ran_rc) return;
    already_ran_rc = 1;

    FILE *file = fopen(".cseshellrc", "r");
    if (!file) return;

    char *commands[MAX_RC_LINES];
    int command_count = 0;
    char line[MAX_LINE_LEN];

    while (fgets(line, sizeof(line), file) && command_count < MAX_RC_LINES) {
        line[strcspn(line, "\n")] = '\0';
        char *start = line;
        while (*start == ' ' || *start == '\t') start++;
        char *end = start + strlen(start) - 1;
        while (end > start && (*end == ' ' || *end == '\t')) {
            *end = '\0'; end--; }
        if (strlen(start) == 0) continue;
        commands[command_count++] = strdup(start);
    }
    fclose(file);

    for (int k = 0; k < command_count; k++) {
        if (strncmp(commands[k], "PATH=", 5) == 0) {
            setenv("PATH", commands[k] + 5, 1);
            free(commands[k]);
            continue;
        }

        //allow set command for customization
        if (strncmp(commands[k], "set ", 4) == 0) {
  	    char *eq = strchr(commands[k], '=');
  	    if (eq) {
    		*eq = '\0';
    		char *key = commands[k] + 4;       // after “set ”
    		char *value = eq + 1;
    		if (strcmp(key, "prompt_format") == 0) {
      		    config.prompt_format = strdup(value);
    		} else if (strcmp(key, "color_scheme") == 0) {
      		    config.color_scheme = strdup(value);
    		} else if (strcmp(key, "show_timestamp") == 0) {
      		    config.show_timestamp = atoi(value);
    		}
  	    }
  	    continue;  // skip fork/exec for “set” lines
	}


        char *args[64]; int i = 0;
        char *token = strtok(commands[k], " ");
        while (token && i < 63) {
            args[i++] = token;
            token = strtok(NULL, " ");
        }
        args[i] = NULL;
        if (!args[0]) { free(commands[k]); continue; }

        //snapshot of usage
        getrusage(RUSAGE_CHILDREN, &prev_usage);
        pid_t pid = fork();
        if (pid == 0) {
            execvp(args[0], args);
            fprintf(stderr, "cseshell: command not found: %s\n", args[0]);
            exit(1);
        } else if (pid > 0) {
            wait(NULL);
            getrusage(RUSAGE_CHILDREN, &curr_usage);
            char *cmd_name = args[0];

            // find before - after
            double u = (curr_usage.ru_utime.tv_sec  - prev_usage.ru_utime.tv_sec)
             + (curr_usage.ru_utime.tv_usec - prev_usage.ru_utime.tv_usec) / 1e6;
            double s = (curr_usage.ru_stime.tv_sec  - prev_usage.ru_stime.tv_sec)
             + (curr_usage.ru_stime.tv_usec - prev_usage.ru_stime.tv_usec) / 1e6;
            long m = curr_usage.ru_maxrss - prev_usage.ru_maxrss;
            long i = curr_usage.ru_inblock - prev_usage.ru_inblock;
            long o = curr_usage.ru_oublock - prev_usage.ru_oublock;

            // print resource stats
            printf("\n");
            printf("Resource usage for \"%s\":\n", cmd_name);
            printf("  Amount of CPU time in user mode       : %.3f seconds\n", u);
            printf("  Amount of CPU time in kernel mode     : %.3f seconds\n", s);
            printf("  Peak RAM usage (max resident size)    : %ld KB\n", m);
            printf("  Block I/O read operations             : %ld\n", i);
            printf("  Block I/O write operations            : %ld\n", o);
            printf("\n");

           // reset for next command
           prev_usage = curr_usage;
        } else {
            perror("fork failed");
        }
        free(commands[k]);
    }
}

int (*builtin_command_func[])(char **) = {
    &shell_cd, &shell_help, &shell_exit, &shell_usage,
    &list_env, &set_env_var, &unset_env_var, &shell_batman, &shell_cyclops, &shell_squidward, &shell_calc,
};

int num_builtin_functions() {
    return sizeof(builtin_commands) / sizeof(char *);
}

/*void read_command(char **cmd) {
    char line[MAX_LINE]; int count = 0, i = 0;
    char *array[MAX_ARGS], *command_token;
    for (;;) {
        int c = fgetc(stdin);
        line[count++] = (char)c;
        if (c == '\n') break;
        if (count >= MAX_LINE) { printf("Command too long\n"); exit(1); }
    }
    line[count] = '\0';
    if (count == 1) { cmd[0] = NULL; return; }

    command_token = strtok(line, " \n");
    while (command_token) {
        array[i++] = strdup(command_token);
        command_token = strtok(NULL, " \n");
    }
    for (int j = 0; j < i; j++) cmd[j] = array[j];
    cmd[i] = NULL;
}*/
void read_command(char **cmd) {
    struct termios orig, raw;
    char prompt_buf[64] = {0};
    char buf[MAX_LINE]     = {0};
    int  pos = 0, hist_i;

    // 1) enable raw mode
    tcgetattr(STDIN_FILENO, &orig);
    raw = orig;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    hist_i = history_count;  // start "below" the latest history entry

    // 3) read keystroke-by-keystroke
    while (1) {
        char c;
        if (read(STDIN_FILENO, &c, 1) != 1) break;

        // Enter: finish reading
        if (c == '\r' || c == '\n') {
            write(STDIN_FILENO, "\r\n", 2);
            break;
        }
        // Backspace
        else if (c == 127 || c == '\b') {
            if (pos > 0) {
                pos--;
                write(STDOUT_FILENO, "\b \b", 3);
            }
        }
        // Escape sequence? → arrow keys
        else if (c == 0x1B) {
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) != 1) continue;
            if (read(STDIN_FILENO, &seq[1], 1) != 1) continue;

            if (seq[0] == '[') {
        	if (seq[1] == 'A' && hist_i > 0)      hist_i--;
        	else if (seq[1] == 'B' && hist_i < history_count) hist_i++;
        	else continue;

        	// erase the whole line
        	write(STDOUT_FILENO, "\r\x1b[2K", 5);

        	// copy history or blank
        	if (hist_i < history_count) {
            	    strcpy(buf, history[hist_i]);
            	    pos = strlen(buf);
        	} else {
            	    buf[0] = '\0'; pos = 0;
        	}

       		type_prompt();
        	write(STDOUT_FILENO, buf, pos);
    	    }
        }
        // Normal character
        else {
            if (pos < MAX_LINE - 1) {
                buf[pos++] = c;
                write(STDOUT_FILENO, &c, 1);
            }
        }
    }

    // 4) disable raw mode
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);

    // 5) record non-empty line into history
    if (pos > 0 && history_count < MAX_HISTORY) {
        history[history_count++] = strdup(buf);
    }

    // 6) tokenize into cmd[]
    int argc = 0;
    char *tok = strtok(buf, " \n");
    while (tok && argc < MAX_ARGS-1) {
        cmd[argc++] = strdup(tok);
        tok = strtok(NULL, " \n");
    }
    cmd[argc] = NULL;
}

// Draw the prompt according to current config
//void draw_prompt() {
//    char buf[128];
//
//    //Optional timestamp
//    if (config.show_timestamp) {
//        time_t t = time(NULL);
//        struct tm *tm = localtime(&t);
//        strftime(buf, sizeof(buf), "[%H:%M:%S] ", tm);
//        printf("%s", buf);
//    }
//
//    //Colour scheme
//    if (strcmp(config.color_scheme, "solarized") == 0) {
//        printf("\x1b[33m");   // yellow
//    } else if (strcmp(config.color_scheme, "dark") == 0) {
//        printf("\x1b[37m");   // bright white
//    }
//    // otherwise default no colour
//    //Prompt format string
//    printf("%s", config.prompt_format);
//
//    if (strcmp(config.color_scheme, "default") != 0) {
//        printf("\x1b[0m");
//    }
//}
//
//
void type_prompt() {
    static int first = 1;
    if (first) { first = 0; }
    fflush(stdout);
    //printf("$$ ");
    draw_prompt();

}


int main(void) {
    //set initial values for prompt style config
    config.prompt_format  = strdup("\\u@\\w$ ");
    config.color_scheme   = strdup("default");
    config.show_timestamp = 0;

    process_rc_file();
    char *cmd[MAX_ARGS]; pid_t pid; int running = 1;

    static char root_path[2048] = "";
    if (!getcwd(root_path, sizeof(root_path))) { perror("getcwd"); exit(1); }

    // Prepend ./bin to PATH so our programs take precedence
    char *oldpath = getenv("PATH");
    char newpath[4096];
    if (oldpath && strlen(oldpath) > 0) {
        snprintf(newpath, sizeof(newpath), "%s/bin:%s", root_path, oldpath);
    } else {
        snprintf(newpath, sizeof(newpath), "%s/bin", root_path);
    }
    setenv("PATH", newpath, 1);

    while (running) {
        type_prompt();
        read_command(cmd);
        if (!cmd[0]) continue;
        if (!strcmp(cmd[0], "exit")) { running = 0; continue; }

	//built-in for set command
    	if (strcmp(cmd[0], "set") == 0 && cmd[1] && strchr(cmd[1], '=')) {
            char *eq    = strchr(cmd[1], '=');
            *eq         = '\0';
            char *key   = cmd[1];
            char *value = eq + 1;

            if (strcmp(key, "prompt_format") == 0) {
            	free(config.prompt_format);
            	config.prompt_format = strdup(value);
            } else if (strcmp(key, "color_scheme") == 0) {
            	free(config.color_scheme);
            	config.color_scheme = strdup(value);
            } else if (strcmp(key, "show_timestamp") == 0) {
            	config.show_timestamp = atoi(value);
            } else {
            	fprintf(stderr, "Unknown setting “%s”\n", key);
            }
            continue;
    	}

	//built-in for history command
	if (strcmp(cmd[0], "history") == 0) {
    	    for (int i = 0; i < history_count; i++) {
        // print with 1-based index
        	printf("%4d  %s\n", i + 1, history[i]);
    	    }
    	continue;  // skip forking/execution
	}



        // Built-ins
        int is_builtin = 0;
        for (int i = 0; i < num_builtin_functions(); i++) {
            if (!strcmp(cmd[0], builtin_commands[i])) {
                builtin_command_func[i](cmd);
                for (int j = 0; cmd[j]; j++) free(cmd[j]);
                is_builtin = 1; break;
            }
        }
        if (is_builtin) continue;

        // External commands
        //snapshot of resource usage
        getrusage(RUSAGE_CHILDREN, &prev_usage);
        pid = fork();
        if (pid == 0) {
            execvp(cmd[0], cmd);
            fprintf(stderr, "command %s not found\n", cmd[0]);
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            wait(NULL);
            getrusage(RUSAGE_CHILDREN, &curr_usage);
            char *cmd_name = cmd[0];
            // find before - after
            double u = (curr_usage.ru_utime.tv_sec  - prev_usage.ru_utime.tv_sec)
             + (curr_usage.ru_utime.tv_usec - prev_usage.ru_utime.tv_usec) / 1e6;
            double s = (curr_usage.ru_stime.tv_sec  - prev_usage.ru_stime.tv_sec)
             + (curr_usage.ru_stime.tv_usec - prev_usage.ru_stime.tv_usec) / 1e6;
            long m = curr_usage.ru_maxrss - prev_usage.ru_maxrss;
            long i = curr_usage.ru_inblock - prev_usage.ru_inblock;
            long o = curr_usage.ru_oublock - prev_usage.ru_oublock;

            // print resource stats
            printf("\n");
            printf("Resource usage for \"%s\":\n", cmd_name);
            printf("  Amount of CPU time in user mode       : %.3f seconds\n", u);
            printf("  Amount of CPU time in kernel mode     : %.3f seconds\n", s);
            printf("  Peak RAM usage (max resident size)    : %ld KB\n", m);
            printf("  Block I/O read operations             : %ld\n", i);
            printf("  Block I/O write operations            : %ld\n", o);
            printf("\n");


            // reset for next command
            prev_usage = curr_usage;
            for (int i = 0; cmd[i]; i++) free(cmd[i]);
        } else {
            perror("fork failed"); running = 0;
        }
    }
    return 0;
}



// Built-in commands implementation unchanged...


int shell_cd(char **args) {
    static char root_path[2048] = "";
    if (root_path[0] == '\0') {
        if (getcwd(root_path, sizeof(root_path)) == NULL) {
            perror("getcwd");
            return 1;
        }
    }

    if (args[1] == NULL) {
        fprintf(stderr, "cd: expected argument\n");
        return 1;
    }

    if (chdir(args[1]) != 0) {
        perror("cd");
        return 1;
    }

    char current[2048];
    if (getcwd(current, sizeof(current)) == NULL) {
        perror("getcwd");
        return 1;
    }

    if (strncmp(current, root_path, strlen(root_path)) != 0) {
        fprintf(stderr, "cd: access outside root directory is not allowed\n");
        chdir(root_path);
    }

    return 1;
}

int shell_help(char **args) {
    printf("Available built-in commands:\n");
    for (int i = 0; i < num_builtin_functions(); i++) {
        printf("  %s\n", builtin_commands[i]);
    }
    return 1;
}

int shell_exit(char **args) {
    exit(0);
}

int shell_usage(char **args) {
    if (args[1] == NULL) {
        printf("Command not given. Type usage <command>.\n");
        return 1;
    }
    if (strcmp(args[1], "cd") == 0) {
        printf("Type: cd directory_name to change the current working directory of the shell\n");
    } else if (strcmp(args[1], "help") == 0) {
        printf("Type: help for supported commands\n");
    } else if (strcmp(args[1], "exit") == 0) {
        printf("Type: exit to terminate the shell gracefully\n");
    } else if (strcmp(args[1], "env") == 0) {
        printf("Type: env to list all registered env variables\n");
    } else if (strcmp(args[1], "setenv") == 0) {
        printf("Type: setenv ENV VALUE to set a new env variable\n");
    } else if (strcmp(args[1], "unsetenv") == 0) {
        printf("Type: unsetenv VAR to remove this env from the list\n");
    } else {
        printf("The command you gave: %s, is not part of the shell's builtin command\n", args[1]);
    }
    return 1;
}

int list_env(char **args) {
    extern char **environ;
    for (char **env = environ; *env != NULL; env++) {
        printf("%s\n", *env);
    }
    return 1;
}

int set_env_var(char **args) {
    if (args[1] == NULL) {
        return 1;
    }

    char *key = strtok(args[1], "=");
    char *value = strtok(NULL, "");

    if (key == NULL || value == NULL) {
        return 1;
    }

    if (setenv(key, value, 1) != 0) {
        perror("setenv");
    }

    return 1;
}

int unset_env_var(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Usage: unsetenv VAR\n");
        return 1;
    }
    if (unsetenv(args[1]) != 0) {
        perror("unsetenv");
    }
    return 1;
}

int shell_batman(char **args) {
    (void)args;
    printf("          .   .\n");
    printf("          |\\_|\\\n");
    printf("          | a_a\\\n");
    printf("          | | \"]\n");
    printf("      ____| '-\\___\n");
    printf("     /.----.___.-'\\\n");
    printf("    //        _    \\\n");
    printf("   //   .-. (~v~) /|\n");
    printf("  |'|  /\\:  .--  / \\\n");
    printf(" // |-/  \\_/____/\\/~|\n");
    printf("|/  \\ |  []_|_|_] \\ |\n");
    printf("| \\  | \\ |___   _\\ ]_}\n");
    printf("| |  '-' /   '.'  |\n");
    printf("| |     /    /|:  | \n");
    printf("| |     |   / |:  /\\\n");
    printf("| |     /  /  |  /  \\\n");
    printf("| |    |  /  /  |    \\\n");
    printf("\\ |    |/\\/  |/|/\\    \\\n");
    printf(" \\|\\ |\\|  |  | / /\\/\\__\\\n");
    printf("  \\ \\| | /   | |__\n");
    printf("       / |   |____)\n");
    printf("       |_/\n");
    return 1;
}

int shell_cyclops(char **args) {
    (void)args;
    printf("           _......._\n");
    printf("       .-'.'.'.'.'.'.-.\n");
    printf("     .'.'.'.'.'.'.'.'.'..\n");
    printf("    /.'.'               '.\\\n");
    printf("    |.'    _.--...--._     |\n");
    printf("    \\    ._.-.....-._.'   /\n");
    printf("    |     _..- .-. -.._   |\n");
    printf(" .-.'    .   ((@))  .'   '.-.\n");
    printf("( ^ \\      --.   .-'     / ^ )\n");
    printf(" \\  /         .   .       \\  /\n");
    printf(" /          .'     '.  .-    \\\n");
    printf("( _.\\    \\ (_-._.-'_)    /._\\)\n");
    printf(" -' \\   ' .--.          / -'\n");
    printf("     |  / /|_| -._.'\\   |\n");
    printf("     |   |       |_| |   /-.._\n");
    printf(" _..-\\   .--.______.'  |\n");
    printf("      \\       .....     |\n");
    printf("       .  .'      .  /\n");
    printf("         \\           .'\n");
    printf("          -..___..-\n");
    return 1;
}

int shell_squidward(char **args) {
    (void)args;
    printf("        .--'''''''''--.\n");
    printf("     .'      .---.      '.\n");
    printf("    /    .-----------.    \\\n");
    printf("   /        .-----.        \\\n");
    printf("   |       .-.   .-.       |\n");
    printf("   |      /   \\ /   \\      |\n");
    printf("    \\    | .-. | .-. |    /\n");
    printf("     '-._| | | | | | |_.-'\n");
    printf("         | '-' | '-' |\n");
    printf("          \\___/ \\___/\n");
    printf("       _.-'  /   \\  -._\n");
    printf("     .' _.--|     |--._ '.\n");
    printf("    ' _...-|     |-..._ '\n");
    printf("           |     |\n");
    printf("           '.___.'\n");
    printf("             | |\n");
    printf("            _| |_ \n");

    printf("         | |     | |\n");
    printf("         | |     | |\n");
    printf("         | |-----| |\n");
    printf("       ./  |     | |/.\n");
    printf("       |    |     |    |\n");
    printf("       '._.'| .-. |'._.'\n");
    printf("             \\ | /\n");
    printf("             | | |\n");
    printf("             | | |\n");
    printf("             | | |\n");
    printf("            /| | |\\\n");
    printf("          .'_| | |_.\n");
    printf("          . | | | .'\n");
    printf("       .    /  |  \\    .\n");
    printf("      /o.-'  / \\  -.o\\\n");
    printf("     /o  o\\ .'   . /o  o\\\n");
    printf("     .___.'       .___.\n");
    return 1;
}

// the built-in entry point
int shell_calc(char **args) {
  // join args[1..] into one string
  if (!args[1]) {
    fprintf(stderr, "Usage: calc <expression>\n");
    return 1;
  }
  // build a single expression string
  char expr[MAX_LINE_LEN] = {0};
  for (int i = 1; args[i]; i++) {
    strcat(expr, args[i]);
    if (args[i+1]) strcat(expr, " ");
  }
  calc_p = expr;
  double result = parse_expr();
  printf("%g\n", result);
  return 1;
}
