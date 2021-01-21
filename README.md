# airlineClerk-sim
## Description

Program: ACS
Author: Nicola Fletcher - 10/20/20

A program created to gain experience using the pthread library 

The executable is called "ACS". ACS simulates an airline check-in system. There are 4 clerks to serve the customers
provided in the input file. The customers can be either economy class or business class. The clerks prioritize business
class, and will only serve economy customers if the business queue is empty. ACS outputs events and state changes as
the customers are being served. These include a customer arriving, a customer entering a queue, clerk beginning service,
and a clerk finishing service.

Once all the customers have been served, wait time statistics are printed to the terminal.

A sample input file "customers.txt" has been provided

## Usage 


    * Compilation: There is a Makefile provided to take care of building the code, which also requires
                   cQueue.c, cQueue.h, and customer_info.h to run. Simply run "make" to compile and create the
                   executable ACS. To fully recompile if changes are made, run "make clean" first.

    * To run: $ ./ACS <input_file.txt>


## Input Format


The input file is defined as follows:

    * Must be a text file (.txt) located in the current directory

    *The contents of the file must follow this exact format:

            - The first line is the number of customers in the file

            - every following line is in the format

                    w:x,y,z

              where 'w' is the customer ID
                    'x' is the class type (0 for economy, 1 for business)
                    'y' is the arrival time (in 10ths of a second)
                    'z' is the service time (in 10ths of a second)

            - every customer line ends with a newline (\n)
