// Nicola Fletcher - 10/20/20

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include "customer_info.h"
#include "cQueue.h"

void * customer_entry(void * cus_info);
void * service();
double getCurrentSimulationTime();
int check_token(char *token);

//global variables
cQueue *queues[2];
cQueue *economy;
cQueue *business;

struct timeval init_time; //to hold the simulation start time
double overall_waiting_time; //sum of waiting times of all the customers
int num_customers; //total number of customers, given by the first line of the input
int num_business; //to determine total number of customers that were business
int num_economy; //to determine total number of customers that were economy
double waiting_times[2]; //array that holds the queue-specific sum of waiting times (0-economy, 1-business)
int served_customers = 0; //keeps track of customers that have finished their service
int available_clerks = 4; //keeps track of clerks available to serve customers
char clerk_status[4] = {0,0,0,0}; //0 for available, 1 for busy
int queue_length[2]; //variable stores the real-time queue length information

//mutex locks
pthread_mutex_t economy_lock;
pthread_mutex_t business_lock;
pthread_mutex_t available_clerks_lock;
pthread_mutex_t clerk_status_lock;
pthread_mutex_t q_length_lock;
pthread_mutex_t overall_waiting_time_lock;
pthread_mutex_t num_business_lock;
pthread_mutex_t num_economy_lock;
pthread_mutex_t waiting_times_lock;

//condition variable
pthread_cond_t convar = PTHREAD_COND_INITIALIZER;

int main(int argc, char *argv[]) {

    //check if input file has been provided
    if(argc < 2){
        printf("Please provide a customers.txt input file\n");
        return -1;
    }
    //check if input file is correct format (.txt)
    char *ext = strrchr(argv[1], '.');
    if(!ext || (strcmp(ext, ".txt") != 0)){
        printf("Input file must have the extension .txt\n");
        return -1;
    }

    //initialize mutex locks
    if (pthread_mutex_init(&economy_lock, NULL) != 0){
        fprintf(stderr, "Error: Mutex init failed\n");
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_init(&business_lock, NULL) != 0){
        fprintf(stderr, "Error: Mutex init failed\n");
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_init(&q_length_lock, NULL) != 0){
        fprintf(stderr, "Error: Mutex init failed\n");
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_init(&overall_waiting_time_lock, NULL) != 0){
        fprintf(stderr, "Error: Mutex init failed\n");
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_init(&available_clerks_lock, NULL) != 0){
        fprintf(stderr, "Error: Mutex init failed\n");
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_init(&clerk_status_lock, NULL) != 0){
        fprintf(stderr, "Error: Mutex init failed\n");
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_init(&num_business_lock, NULL) != 0){
        fprintf(stderr, "Error: Mutex init failed\n");
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_init(&num_economy_lock, NULL) != 0){
        fprintf(stderr, "Error: Mutex init failed\n");
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_init(&waiting_times_lock, NULL) != 0){
        fprintf(stderr, "Error: Mutex init failed\n");
        exit(EXIT_FAILURE);
    }

    //initialize queues
    economy = (cQueue*) malloc(sizeof(cQueue));
    economy->head = NULL;
    economy->tail = NULL;
    economy->size = 0;

    business = (cQueue*) malloc(sizeof(cQueue));
    business->head = NULL;
    business->tail = NULL;
    business->size = 0;

    queues[0] = economy;
    queues[1] = business;

    //open customer.txt file
    FILE * input = fopen(argv[1], "r");
    if(input == NULL){
        fprintf(stderr, "Error: could not open customer file\n");
        exit(EXIT_FAILURE);
    }

    char line[256];

    //read first line of file (aka number of customers)
    fgets(line, sizeof(line), input);

    //check for illegal input (negative number or non-number)
    if(check_token(line) == 0){
        fprintf(stderr, "Error: invalid input\n");
        fclose(input);
        exit(EXIT_FAILURE);
    }

    num_customers = atoi(line);

    //customer information
    struct customer_info custom_info[num_customers];
    pthread_t customId[num_customers];

    int count = 0;

    //read remaining lines of file and insert the info for each customer into the array
    while(fgets(line, sizeof(line), input)){

        customer_info new_customer;
        char *token = strtok(line, ":,\n");

        //customer number
        if(check_token(token) == 0){
            fprintf(stderr, "Error: invalid input\n");
            fclose(input);
            exit(EXIT_FAILURE);
        }

        new_customer.user_id = atoi(token);

        //class (aka queue number)
        token = strtok(NULL, ":,\n");
        //additional check for if queue # is not 0 or 1
        if((check_token(token) == 0 || (atoi(token) != 0) && (atoi(token) != 1))){
            fprintf(stderr, "Error: invalid input\n");
            fclose(input);
            exit(EXIT_FAILURE);
        }

        new_customer.class_type = atoi(token);

        //arrival time
        token = strtok(NULL, ":,\n");
        if(check_token(token) == 0){
            fprintf(stderr, "Error: invalid input\n");
            fclose(input);
            exit(EXIT_FAILURE);
        }

        new_customer.arrival_time = atoi(token);

        //service time
        token = strtok(NULL, ":,\n");
        if(check_token(token) == 0){
            fprintf(stderr, "Error: invalid input\n");
            fclose(input);
            exit(EXIT_FAILURE);
        }

        new_customer.service_time = atoi(token);

        custom_info[count] = new_customer;
        count++;
    }

    fclose(input);

    fprintf(stdout, "\nBEGINNING SIMULATION\n");
    sleep(1);

    //record simulation start time
    gettimeofday(&init_time, NULL);

    //create customer threads
    int rc;
    for(int j = 0; j < num_customers; j++) {

        rc = pthread_create(&customId[j], NULL, customer_entry, (void *) &custom_info[j]);

        if(rc != 0){
            fprintf(stderr, "Error: pthread_create, rc: %d\n", rc);
            exit(EXIT_FAILURE);
        }
    }

    for (int k = 0; k < num_customers; k++) {
        pthread_join(customId[k], NULL);
    }

    //calculate + output the waiting time statistics

    //avoid division by 0 if there were no customers in one or both of the queues
    if(num_customers == 0){
        num_customers = 1;
    }
    if(num_business == 0){
        num_business = 1;
    }
    if(num_economy == 0){
        num_economy = 1;
    }

    double wait_avg = overall_waiting_time / num_customers;
    double business_wait_avg = waiting_times[1] / num_business;
    double economy_wait_avg = waiting_times[0] / num_economy;

    fprintf(stdout, "\nWAITING TIME STATISTICS\n");
    fprintf(stdout, "The average waiting time for all customers in the system is: %.2f seconds.\n", wait_avg);
    fprintf(stdout, "The average waiting time for all business-class customers is: %.2f seconds.\n", business_wait_avg);
    fprintf(stdout, "The average waiting time for all economy-class customers is: %.2f seconds.\n", economy_wait_avg);

    //free queue memory
    free(economy);
    free(business);

    return 0;
}

//simulates the customer arrival time and adds the customer to the correct queue
void * customer_entry(void * cus_info) {

    customer_info *info = (customer_info *) cus_info;

    usleep(info->arrival_time * 100000);

    fprintf(stdout, "A customer arrives: customer ID %2d. \n", info->user_id);

    //add the customer to the correct queue
    if (info->class_type == 1) {

        pthread_mutex_lock(&business_lock);

        fprintf(stdout, "A customer enters a queue: the queue ID %1d, and length of the queue %2d. \n",
                info->class_type, queue_length[info->class_type]);

        //get the time they enter the queue
        double start_wait = getCurrentSimulationTime();
        //add the customer to the queue
        enqueue(queues[info->class_type], *info, start_wait);

        pthread_mutex_unlock(&business_lock);

        //increment business customer counter
        pthread_mutex_lock(&num_business_lock);
        num_business++;
        pthread_mutex_unlock(&num_business_lock);

    } else {
        pthread_mutex_lock(&economy_lock);

        fprintf(stdout, "A customer enters a queue: the queue ID %1d, and length of the queue %2d. \n",
                info->class_type, queue_length[info->class_type]);

        //get the time they enter the queue
        double start_wait = getCurrentSimulationTime();
        //add the customer to the queue
        enqueue(queues[info->class_type], *info, start_wait);

        pthread_mutex_unlock(&economy_lock);

        //increment economy customer counter
        pthread_mutex_lock(&num_economy_lock);
        num_economy++;
        pthread_mutex_unlock(&num_economy_lock);
    }

    //update length of queue
    pthread_mutex_lock(&q_length_lock);
    queue_length[info->class_type]++;
    pthread_mutex_unlock(&q_length_lock);

    //activate clerks serving the customers
    service();

    pthread_exit(NULL);
    return NULL;

}

//simulates all of the customers being served by 4 clerks
void * service(){

    //check if a clerk is available
    pthread_mutex_lock(&available_clerks_lock);
    //if there are no clerks currently available--> wait for one to become available
    while (available_clerks == 0) {
        pthread_cond_wait(&convar, &available_clerks_lock);
    }

    //clerk that had become available will now become busy again
    available_clerks--;

    pthread_mutex_unlock(&available_clerks_lock);

    //find the available clerk
    pthread_mutex_lock(&clerk_status_lock);

    int clerk_id = 0;
    for (int i = 0; i < 4; i++) {
        if (clerk_status[i] == 0) {
            clerk_id = i + 1;
            clerk_status[i] = 1; //clerk is now busy
            break;
        }
    }

    pthread_mutex_unlock(&clerk_status_lock);

    pthread_mutex_lock(&business_lock);
    pthread_mutex_lock(&economy_lock);

    //remove customer to be served from the correct queue
    Node* served_customer;

    while (served_customers < num_customers) {

        //priority goes to business queue (only take customers from economy if business is empty)
        if (business->head != NULL) {
            served_customer = dequeue(business);
            break;
        }
        else if(economy->head != NULL) {
            served_customer = dequeue(economy);
            break;
        }
    }

    //update length of queue
    pthread_mutex_lock(&q_length_lock);
    queue_length[served_customer->info.class_type]--;
    pthread_mutex_unlock(&q_length_lock);

    pthread_mutex_unlock(&business_lock);
    pthread_mutex_unlock(&economy_lock);

    //get the time that a customer finishes waiting and begins their service
    double end_wait = getCurrentSimulationTime();
    //calculate the wait time (time they spent in the queue) of the customer being served
    double wait_time = end_wait - (served_customer->entry_time);
    //add the customers wait time to the overall wait time counter
    pthread_mutex_lock(&overall_waiting_time_lock);
    overall_waiting_time += wait_time;
    pthread_mutex_unlock(&overall_waiting_time_lock);

    //add the customers wait time to the relevant class wait time counter
    pthread_mutex_lock(&waiting_times_lock);
    waiting_times[served_customer->info.class_type] += wait_time;
    pthread_mutex_unlock(&waiting_times_lock);

    fprintf(stdout, "A clerk starts serving a customer: start time %.2f, the customer ID %2d, the clerk ID %1d. \n", end_wait, served_customer->info.user_id, clerk_id);

    //service time for the customer
    usleep(served_customer->info.service_time*100000);

    double finish_time = getCurrentSimulationTime();

    fprintf(stdout, "A clerk finishes serving a customer: end time %.2f, the customer ID %2d, the clerk ID %1d. \n", finish_time, served_customer->info.user_id, clerk_id);

    //change clerk status to available and increment served_customers
    pthread_mutex_lock(&clerk_status_lock);
    clerk_status[clerk_id - 1] = 0;
    served_customers++;
    pthread_mutex_unlock(&clerk_status_lock);

    //increment available_clerks and notify waiting threads that a clerk is now available
    pthread_mutex_lock(&available_clerks_lock);
    available_clerks++;
    pthread_cond_broadcast(&convar);
    pthread_mutex_unlock(&available_clerks_lock);

    //free customer memory
    free(served_customer);

    return NULL;

}
//returns the current simulation time in seconds - provided for this assignment by Huan Wang
double getCurrentSimulationTime(){

    struct timeval cur_time;
    double cur_secs, init_secs;

    //pthread_mutex_lock(&start_time_mtex); you may need a lock here
    init_secs = (init_time.tv_sec + (double) init_time.tv_usec / 1000000);
    //pthread_mutex_unlock(&start_time_mtex);

    gettimeofday(&cur_time, NULL);
    cur_secs = (cur_time.tv_sec + (double) cur_time.tv_usec / 1000000);

    return cur_secs - init_secs;
}

//determines whether a token from the input file is valid input
int check_token(char *token){

    for(char *curr = token; *curr != '\0'; curr++ ){

        if((isdigit(*curr) == 0) || (atoi(curr) < 0)){
            if(strcmp(curr, "\n") == 0){
                return 1;
            }
            return 0;
        }
    }
    return 1;

}