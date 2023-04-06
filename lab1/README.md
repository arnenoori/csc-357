struct ListElement: defines the list element, which contains a pointer to the next and previous elements, and a character array to store the string.

ListElement *head: A pointer to the head (first element) of the list initialized empty

getLastElement(): returns a pointer to the last element of the list.

pushString(const char *str): takes a string as input and adds it to the end of the list.

printList(): prints all the strings in the list.

deleteItem(int itemNumber): takes an item number as input and deletes the corresponding element from the list.

deleteList(): deletes the entire list and sets the head to NULL.

main(): presents a menu to the user, allowing them to interact with the list. The user can choose to:
a. Add a string to the list
b. Print the list
c. Delete an item from the list
d. End the program, which also deletes the entire list before exiting