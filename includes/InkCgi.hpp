/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   InkCgi.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: oel-ouar <oel-ouar@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/09/27 12:15:28 by oel-ouar          #+#    #+#             */
/*   Updated: 2022/02/15 08:04:28 by                  ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once


#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <cstdio>
#include <sys/wait.h>
#include "InkRequest.hpp"
#include "InkServerConfig.hpp"
#include <cerrno>
#include <signal.h>

extern  char** environ;

namespace  ft {
class InkCgi
{
    private:
        char **env;
        char *_lang;
        std::string scriptName;
    
    public:
        InkCgi(request req, ft::Location serverConf)
        {
			if (req.getPath() == "/")
				scriptName = serverConf.getRoot() + req.getPath()+req.getScriptName();
			else
				scriptName = serverConf.getRoot() + req.getPath()+ "/" + req.getScriptName();
            init_env(req);
        }
        ~InkCgi()
        {
            
        }
        
        void init_env(request req)
        {
            if(req.getQuery().length() != 0)
                setenv("CONTENT_LENGTH", To_string(req.getQuery().length()).c_str(), 1);
            if (req.getDetails().find("Content-Type") != req.getDetails().end())
                setenv("CONTENT_TYPE", (req.getDetails().find("Content-Type"))->second.c_str(), 1);
            else
                setenv("CONTENT_TYPE", "text/html; charset=UTF-8", 1);
            if (req.getDetails().find("Cookie") != req.getDetails().end())
                setenv("HTTP_COOKIE", (req.getDetails().find("Cookie"))->second.c_str(), 1);
            setenv("GATEWAY_INTERFACE", "CGI/1.1", 1);
            setenv("QUERY_STRING", req.getQuery().c_str(), 1);
            setenv("REQUEST_METHOD", req.getMethod().c_str(), 1);
            setenv("SCRIPT_FILENAME", scriptName.c_str(), 1);
            setenv("SERVER_SOFTWARE", "INK", 1);
            setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
            setenv("REDIRECT_STATUS", "true", 1);
            if (req.getScriptName().find(".php") != std::string::npos)
                _lang = strdup("/Users/oel-ouar/.brew/bin/php-cgi");
            else if (req.getScriptName().find(".py") != std::string::npos)
                _lang = strdup("/usr/bin/python");
        }
        
        std::pair<std::string,std::string> execCgi(request req)
        {
            char *args[3];
            int pid;
            int stat;
            bool done = true;
            std::string cgibuffer ="";
            FILE  *file;
            char a;
            args[0] =_lang;
            args[1] = (char*)scriptName.c_str();
            args[2] = NULL;
            int fd[2];
            int fd1[2];
            pipe(fd);
            pipe(fd1);
            pid = fork();
            if (pid == -1)
                return (std::make_pair("500", "HTTP/1.1 500 Internal Server Error\r\n"));
            else if (pid == 0)
            {
                if (req.getMethod() == "POST")
                {
                    dup2(fd1[0],0);
                    write(fd1[1], req.getQuery().c_str(), req.getQuery().length());
                }
                else
                    close(0);
                close(fd1[0]);
                close(fd1[1]);
                dup2(fd[1], 1);
                close(fd[1]);
                close(fd[0]);
                execve(args[0], args, environ);
            }
            else
            {
                time_t t = time(NULL);
                while ( time(NULL) - t < 5) {
                    pid_t r = waitpid(pid, &stat, WNOHANG);
					std::cout << r << std::endl;
                    if (r && r!=-1) 
                    {
                        close(fd[1]);
                        close(fd1[0]);
                        close(fd1[1]);
                        file = fdopen(fd[0],"r");
                        while ((a = fgetc(file)) != EOF)
                            cgibuffer += a;
                        fclose(file);
                        close(fd[0]);
                        done = false;
                        break;
                    }
                }
                if (done)
                {
                    kill(pid, SIGKILL);
                    close(fd[1]);
                    close(fd1[0]);
                    close(fd1[1]);
                    return(std::make_pair("500", "HTTP/1.1 500 Internal Server Error\r\n"));
                }
            }
            if (cgibuffer[0]=='5')
                return (std::make_pair("500", "HTTP/1.1 500 Internal Server Error\r\n"));
            if (cgibuffer.find("Status: 302") != std::string::npos)
                return (std::make_pair(cgibuffer, "HTTP/1.1 302 Found\r\n"));
            else if (cgibuffer.find("Status: 301") != std::string::npos)
                return (std::make_pair(cgibuffer, "HTTP/1.1 301 Moved Permanetly\r\n"));
            return (std::make_pair(cgibuffer, "HTTP/1.1 200 OK\r\n"));
        }
    private:
        char *join(const char *s1, const char *s2)
        {
            char *result = (char *)malloc(strlen(s1) + strlen(s2) + 1);
    
            if (result)
            {
                strcpy(result, s1);
                strcat(result, s2);
            }
            return result;
        }
        template<class T>
        std::string To_string(T n){
            std::ostringstream convert;
            convert << n;
            return(convert.str());
        }
};

}
