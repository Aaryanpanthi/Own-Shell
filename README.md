<!DOCTYPE html>
<html>
<head>
<title>Quash: A Simple UNIX-Like Shell</title>
<style>
body {
    font-family: Arial, sans-serif;
    margin: 40px;
    line-height: 1.5;
}
h1, h2, h3 {
    border-bottom: 1px solid #ccc;
    padding-bottom: 5px;
}
pre {
    background: #f4f4f4;
    padding: 10px;
    border-radius: 5px;
}
code {
    background: #f9f9f9;
    padding: 2px 4px;
    border-radius: 3px;
}
</style>
</head>
<body>

<h1>Quash: A Simple UNIX-Like Shell</h1>

<p><strong>Author:</strong> Your Name<br>
<strong>Teammate (if any):</strong> Teammate’s Name</p>

<h2>Overview</h2>

<p>Quash is a simplified UNIX-like command shell implemented in C. It provides a command-line interface (CLI) for users to execute both built-in and external commands. Quash demonstrates fundamental concepts of operating systems such as process forking, I/O redirection, signal handling, and environment variable manipulation.</p>

<p>This project was developed as a course assignment to deepen understanding of system calls, signals, environment variable handling, and the internal workings of a shell.</p>

<h2>Features</h2>

<ul>
  <li><strong>Prompt:</strong> Displays the current working directory as a prompt (e.g., <code>/home/user></code>).</li>
  <li><strong>Built-in Commands:</strong> Supports <code>cd</code>, <code>pwd</code>, <code>echo</code>, <code>env</code>, <code>setenv</code>, and <code>exit</code>.</li>
  <li><strong>Environment Variable Expansion:</strong> Recognizes <code>$VAR</code> and replaces it with the value of the environment variable.</li>
  <li><strong>Process Execution:</strong> Forks and executes external programs specified by the user.</li>
  <li><strong>Background Processes:</strong> Appending <code>&</code> to a command runs the process in the background.</li>
  <li><strong>Signal Handling:</strong> Properly handles <code>SIGINT</code> (Ctrl-C) so that it does not terminate the shell itself.</li>
  <li><strong>Timeout for Foreground Processes:</strong> Kills any foreground process exceeding a 10-second runtime.</li>
  <li><strong>I/O Redirection:</strong> Allows output redirection using <code>></code>.</li>
</ul>

<h2>Compilation and Execution</h2>

<p>Use the provided Makefile to compile Quash:</p>
<pre><code>make
./shell
</code></pre>

<p>You will then be presented with the shell prompt.</p>

<h2>Example Usage</h2>

<pre><code>/home/user> pwd
/home/user

/home/user> echo hello world
hello world

/home/user> echo $HOME
/home/user

/home/user> cd test
/home/user/test> pwd
/home/user/test

/home/user/test> env
PATH=/usr/local/bin:/usr/bin:/bin
HOME=/home/user
...

/home/user/test> setenv GREETING=hello
/home/user/test> echo $GREETING
hello

/home/user/test> ls > output.txt
/home/user/test> cat output.txt
file1
file2
shell.c

/home/user/test> a_long_running_program
# If it runs longer than 10 seconds, it will be killed automatically

/home/user/test> a_program &
Started background process with PID: 12345

/home/user/test> exit
</code></pre>

<hr>

<h1>Design and Documentation Report</h1>

<h2>1. Introduction</h2>

<p>The goal of this project is to implement a simplified shell, named Quash, to reinforce the concepts taught in operating systems and systems programming. By creating a shell, we gain hands-on experience with how commands are parsed, executed, and managed in a UNIX-like environment. The project also involves ensuring the shell can handle signals gracefully, manage environment variables, and execute processes both synchronously (foreground) and asynchronously (background).</p>

<h2>2. Design Choices</h2>

<h3>2.1 Overall Architecture</h3>

<p>Quash follows a straightforward design:</p>

<ol>
  <li><strong>Main Loop:</strong> Continuously print a prompt, read user input, and parse it into tokens.</li>
  <li><strong>Tokenization &amp; Parsing:</strong> Use <code>strtok()</code> to split the input line by delimiters. The first token is the command, subsequent tokens are arguments.</li>
  <li><strong>Built-in Command Handling:</strong> Check if the command is built-in. If so, handle it internally.</li>
  <li><strong>External Commands:</strong> If not built-in, fork a new process and call <code>execvp()</code> to execute. The parent waits unless running in the background.</li>
  <li><strong>Environment Variables:</strong> Expand variables starting with <code>$</code> using <code>getenv()</code>.</li>
  <li><strong>Redirection &amp; Timer:</strong> Use <code>dup2()</code> for <code>></code> redirection. Use <code>alarm(10)</code> and a <code>SIGALRM</code> handler to enforce a 10-second limit on foreground processes.</li>
  <li><strong>Signal Handling:</strong> The shell ignores <code>SIGINT</code>, preventing Ctrl-C from killing the shell, but allows the child to be killed.</li>
</ol>

<h3>2.2 Data Structures</h3>
<ul>
  <li><strong>Arguments Array:</strong> <code>char *arguments[MAX_COMMAND_LINE_ARGS]</code> stores parsed tokens.</li>
  <li><strong>Global Variables:</strong> 
    <ul>
      <li><code>extern char **environ;</code> for environment variables.</li>
      <li><code>static pid_t current_foreground_pid</code> tracks the current foreground process.</li>
    </ul>
  </li>
  <li><strong>String Buffers:</strong> <code>command_line</code> buffer for raw input.</li>
</ul>

<h3>2.3 Handling Built-in Commands</h3>

<p><strong>cd:</strong> Uses <code>chdir()</code>.<br>
<strong>pwd:</strong> Uses <code>getcwd()</code>.<br>
<strong>echo:</strong> Prints arguments. Replaces <code>$VAR</code> with the variable’s value.<br>
<strong>env:</strong> Prints environment variables or specific variable value.<br>
<strong>setenv:</strong> Uses <code>setenv()</code> to create/update variables.<br>
<strong>exit:</strong> Terminates the shell.</p>

<h3>2.4 Executing External Commands</h3>

<ol>
  <li><strong>Fork Child:</strong> Child resets signals and redirects I/O if needed.</li>
  <li><strong>execvp():</strong> Replace child’s image with the program.</li>
  <li><strong>Parent Waits:</strong> For foreground processes, use <code>waitpid()</code>. Background processes do not block the shell.</li>
</ol>

<h3>2.5 Timing Out Foreground Processes</h3>

<p>When a foreground process starts, <code>alarm(10)</code> sets a 10-second timer. The <code>SIGALRM</code> handler kills the process if it times out. If the process finishes early, <code>alarm(0)</code> cancels the timer.</p>

<h3>2.6 I/O Redirection</h3>

<p>On encountering <code>></code>, open the file and use <code>dup2()</code> in the child to redirect stdout to the file. Then execute the program via <code>execvp()</code>.</p>

<h3>2.7 Signal Handling</h3>

<ul>
  <li><strong>SIGINT:</strong> The shell prints a newline on Ctrl-C and remains alive. Child processes use default handling, so Ctrl-C can kill them if desired.</li>
  <li><strong>SIGALRM:</strong> Used for timing out long-running processes.</li>
</ul>

<h2>3. Testing</h2>

<p>We tested the shell extensively:</p>
<ul>
  <li><strong>Built-in commands:</strong> Verified <code>cd</code>, <code>pwd</code>, <code>echo</code>, <code>env</code>, and <code>setenv</code>.</li>
  <li><strong>External commands:</strong> Tested with <code>ls</code>, <code>cat</code>, <code>grep</code> to ensure proper forking and execution.</li>
  <li><strong>Background processes:</strong> Commands like <code>sleep 5 &amp;</code> run without blocking the shell.</li>
  <li><strong>Signal handling:</strong> Running <code>sleep 100</code> and pressing Ctrl-C only kills the child process, not the shell.</li>
  <li><strong>Timeout:</strong> Verified with <code>sleep 15</code> that the process is killed after 10 seconds.</li>
  <li><strong>Redirection:</strong> <code>ls > out.txt</code> successfully redirects output to the file.</li>
</ul>

<h2>4. Known Issues / Limitations</h2>

<ul>
  <li><strong>Piping &amp; Input Redirection:</strong> Only <code>></code> is implemented. No <code>|</code> or <code>&lt;</code> handling is shown here.</li>
  <li><strong>No Job Control:</strong> No advanced job control features (fg, bg, jobs).</li>
  <li><strong>Memory Management:</strong> Basic usage only. For a full shell, more robust memory handling might be required.</li>
</ul>

<h2>5. Conclusion</h2>

<p>Implementing Quash provided practical experience with core system-level programming tasks. Although simplified, it captures the essence of how shells operate. Potential extensions include implementing more redirections, pipes, and job control to approach a more fully featured shell.</p>

</body>
</html>
