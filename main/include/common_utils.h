#pragma once

#include <stdlib.h>
#include <stdbool.h>

typedef struct node
{
    uint8_t data;
    struct node *next;
} Node;

typedef struct list
{
    Node *head;
} List;

List *makeLinkedList();
void addLLElement(uint8_t data, List *list);
void deleteLLElement(uint8_t data, List *list);
void reverseLinkedList(List *list);
void destroyLinkedList(List *list);
bool existsInLL(uint8_t data, List *list);