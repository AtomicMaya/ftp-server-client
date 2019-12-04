/** @author Maya B., hasancse91 for the original linked list implementation */
#include <stdlib.h>
#include <stdio.h>

#include "llist.h"

node *head = NULL, *last = NULL;

void insert_at_last(int value) {
    node *temp_node;
    temp_node = (node *) malloc(sizeof(node));

    temp_node->number = value;
    temp_node->next = NULL;

    //For the 1st element
    if(head == NULL) {
        head = temp_node;
        last = temp_node;
    } else {
        last->next = temp_node;
        last = temp_node;
    }
}

void delete_item(int value) {
    node *myNode = head, *previous = NULL;
    while(myNode != NULL) {
        if(myNode->number == value) {
            if(previous == NULL)
                head = myNode->next;
            else
                previous->next = myNode->next;

            free(myNode);
            break;
        }

        previous = myNode;
        myNode = myNode->next;
    }
}

int get_first() {
    node *myList;
    myList = head;
    return (myList != NULL) ? myList->number : -1;
}