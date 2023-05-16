/* Author: Sam Fletcher
 * Program: Lab2 - PSUsh
 * Date: 16 May, 2023
 * Description: This is the main file for the psush program.
 * Most of the hard work is done in cmd_parse.c so go over there.
 * Citations:
 * Shamelessly stolen from rchaney@pdx.edu
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cmd_parse.h"

//Dont get mad at me, I'm using your global.
extern unsigned short is_verbose;

int main(int argc, char *argv[])
{
    int ret = 0;

    simple_argv(argc, argv);
    ret = process_user_input_simple();
    return ret;
}