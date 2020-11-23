/*******************************************************************************
 * libretroshare/src/retroshare/util/rskbdinput.cc                             *
 *                                                                             *
 * Copyright (C) 2019  Cyril Soler <csoler@users.sourceforge.net>              *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#ifndef __ANDROID__

#include <iostream>
#include <util/rskbdinput.h>

#ifdef WINDOWS_SYS
#include <conio.h>
#include <stdio.h>

#define PASS_MAX 512

namespace RsUtil {
std::string rs_getpass(const std::string& prompt,bool no_echo)
{
    static char getpassbuf [PASS_MAX + 1];
    size_t i = 0;
    int c;

    if (!prompt.empty()) {
        std::cerr << prompt ;
        std::cerr.flush();
    }

    for (;;) {
        c = _getch ();
        if (c == '\r') {
            getpassbuf [i] = '\0';
            break;
        }
        else if (i < PASS_MAX) {
            getpassbuf[i++] = c;
        }

        if (i >= PASS_MAX) {
            getpassbuf [i] = '\0';
            break;
        }
    }

    if (!prompt.empty()) {
        std::cerr << "\r\n" ;
        std::cerr.flush();
    }

    return std::string(getpassbuf);
}
}
#else

#include <stdio.h>
#include <string>
#include <iostream>
#include <termios.h>
#include <unistd.h>

static int getch()
{
    int ch;
    struct termios t_old, t_new;

    tcgetattr(STDIN_FILENO, &t_old);
    t_new = t_old;
    t_new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t_new);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &t_old);
    return ch;
}

namespace RsUtil {

std::string rs_getpass(const std::string& prompt, bool no_echo)
{
  const char BACKSPACE=127;
  const char RETURN=10;

  std::string password;
  unsigned char ch=0;

  std::cout <<prompt; std::cout.flush();

  while((ch=getch())!=RETURN)
    {
       if(ch==BACKSPACE)
         {
            if(password.length()!=0)
              {
                 if(no_echo)
                 std::cout <<"\b \b";
                 password.resize(password.length()-1);
              }
         }
       else
         {
             password+=ch;
             if(no_echo)
                 std::cout <<'*';
             else
                 std::cout << ch,std::cout.flush();
         }
    }
  std::cout <<std::endl;

  return std::string(password);
}
}
#endif
#endif
