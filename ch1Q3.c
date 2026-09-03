#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// doubly linked list
// defining a node
typedef struct Node {
    char *data;
    struct Node *next;
    struct Node *prev;
} Node;

// create a new node
Node* createNode(const char *data){
    Node *newNode = malloc(sizeof(Node));

    if (newNode == NULL) {
        return NULL;
    }

    newNode->data = malloc(strlen(data) + 1);

    if (newNode->data == NULL) {
        free(newNode);
        return NULL;
    }

    strcpy(newNode->data, data);

    newNode->next = NULL;
    newNode->prev = NULL;

    return newNode;
}

// insert beginning
void insertAtBeginning(Node** head, const char *data)
{
    // creating new node
    Node* newNode = createNode(data);

    // check if DLL is empty
    if (*head == NULL) {
        *head = newNode;
        return;
    }
    newNode->next = *head;
    (*head)->prev = newNode;
    *head = newNode;
}

// insert at the end
void insertAtEnd(Node** head, const char *data)
{
    // creating new node
    Node* newNode = createNode(data);

    // check if DLL is empty
    if (*head == NULL) {
        *head = newNode;
        return;
    }

    Node* temp = *head;
    while (temp->next != NULL) {
        temp = temp->next;
    }
    temp->next = newNode;
    newNode->prev = temp;
}

// delete beginning
void deleteAtBeginning(Node** head)
{
    // checking if the DLL is empty
    if (*head == NULL) {
        printf("The list is already empty.\n");
        return;
    }
    Node* temp = *head;
    *head = (*head)->next;
    if (*head != NULL) {
        (*head)->prev = NULL;
    }
    free(temp->data);
    free(temp);
}

// delete end
void deleteAtEnd(Node** head)
{
    // checking if DLL is empty
    if (*head == NULL) {
        printf("The list is already empty.\n");
        return;
    }

    Node* temp = *head;
    if (temp->next == NULL) {
        *head = NULL;
        free(temp->data);
        free(temp);
        return;
    }
    while (temp->next != NULL) {
        temp = temp->next;
    }
    temp->prev->next = NULL;
    free(temp->data);
    free(temp);
}

// print list 
void printList(Node* head)
{
    Node* temp = head;
    while (temp != NULL) {
        printf("%s ", temp->data);
        temp = temp->next;
    }
    printf("\n");
}

int main() {

    // hello world
    printf("Hello, World!");
    printf("\n");
    
    // creating node and testing its functions
    Node* dll = NULL;

    insertAtBeginning(&dll, "cat");
    insertAtBeginning(&dll, "dog");
    insertAtEnd(&dll, "fish");

    printf("List: ");
    printList(dll);
    
    deleteAtBeginning(&dll);

    printf("List after getting rid of the dog: ");  
    printList(dll);  

    return 0;
}