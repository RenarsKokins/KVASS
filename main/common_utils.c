#include "esp_log.h"

#include "common_utils.h"

Node *createnode(uint8_t data)
{
    Node *newNode = malloc(sizeof(Node));
    if (!newNode)
    {
        return NULL;
    }
    newNode->data = data;
    newNode->next = NULL;
    return newNode;
}

List *makeLinkedList()
{
    List *list = malloc(sizeof(List));
    if (!list)
    {
        return NULL;
    }
    list->head = NULL;
    return list;
}

void addLLElement(uint8_t data, List *list)
{
    Node *current = NULL;
    if (list->head == NULL)
    {
        list->head = createnode(data);
    }
    else
    {
        current = list->head;
        while (current->next != NULL)
        {
            current = current->next;
        }
        current->next = createnode(data);
    }
}

void deleteLLElement(uint8_t data, List *list)
{
    Node *current = list->head;
    Node *previous = current;
    while (current != NULL)
    {
        if (current->data == data)
        {
            previous->next = current->next;
            if (current == list->head)
                list->head = current->next;
            free(current);
            return;
        }
        previous = current;
        current = current->next;
    }
}

bool existsInLL(uint8_t data, List *list)
{
    Node *current = list->head;
    while (current != NULL)
    {
        if (current->data == data)
        {
            return true;
        }
        current = current->next;
    }
    return false;
}

void reverseLinkedList(List *list)
{
    Node *reversed = NULL;
    Node *current = list->head;
    Node *temp = NULL;
    while (current != NULL)
    {
        temp = current;
        current = current->next;
        temp->next = reversed;
        reversed = temp;
    }
    list->head = reversed;
}

void destroyLinkedList(List *list)
{
    Node *current = list->head;
    Node *next = current;
    while (current != NULL)
    {
        next = current->next;
        free(current);
        current = next;
    }
    free(list);
}