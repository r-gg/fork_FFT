/**
 * @author Rastko Gajanin, 11930500
 * @brief c File for the Assignment 2 forkFFT
 * @date 19.11.2020
 */


#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#ifndef forkFFT_H_
#define forkFFT_H_

#define MAX_CHARS_PER_FLOAT_NUMBER 50 // maximum number of digits (with - and . included)

typedef struct compNumber
{
    float a;
    float b;
}complexNumber;

char* fileName;
/**
 * @brief function for printing the usage message, upon incorrect use of the program
 * @param string with the name of the file
 * @return prints the usage message and exits the program with EXIT_FAILURE
*/
static void myUsage();


/**
 * @brief multiplies two complex numbers
 * @param z1, z2 -- complex numbers to be multiplied
 * @return complexNumber (not a pointer)
 */
static complexNumber multiply(complexNumber z1, complexNumber z2);

/**
 * @brief sums two complex numbers
 * @param z1, z2 -- complex numbers to be summed
 * @return complexNumber (not a pointer)
 */
static complexNumber add(complexNumber z1, complexNumber z2);


/**
 * @brief counts the number of lines in the given file
 * @return the number n (how many float values (lines) are in the file) (because the float values are separeated by '\n')
 */
static int calculateN(FILE *in);

/**
 * @brief reads the lines of in as float numbers into the array values and returns the number of elements 
 * @param in -- file to be read Not null
 * @param values -- must contain MAXLENGTH elements
 * @returns -1 upon error and number of filled lines if there was no error
 */
static int readInput(FILE *in, complexNumber *values);

int main (int argc, char *argv[]);


#endif
