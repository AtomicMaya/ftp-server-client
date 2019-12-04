/** @author Maya B., hasancse91 for the original linked list implementation */
#ifndef HEADER_LSERVER_LLIST
#define HEADER_LSERVER_LLIST

struct linked_list {
    int number;
    struct linked_list *next;
};

typedef struct linked_list node;

void insert_at_last(int value);
void delete_item(int value);
int get_first();

#endif