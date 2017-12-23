#include<stdlib.h>
#include<stdio.h>
#include<assert.h>

#define MAX_NUMBER_OF_NODES 255

/* stack data structure */
struct stack
{
    void *data;
    struct stack *next;
};

typedef struct stack node_stack;

/* Auxiliary memory stack allocation utilities */
static node_stack StaticPoolStack[MAX_NUMBER_OF_NODES];
static node_stack* head_stack_free_list;

void push_stack_free_list(node_stack ** head, node_stack * new_node)
{
    new_node->data = 0;
    new_node->next = *head;
    *head = new_node;
}
node_stack* pop_stack_free_list(node_stack ** head)
{
    node_stack* retval = 0;
    node_stack * next_node = NULL;

    if (*head == NULL)
        return NULL;

    next_node = (*head)->next;
    retval = *head;
    *head = next_node;

    return retval;
}
void init_stack_free_list()
{
    int index;
    for(index=0; index < MAX_NUMBER_OF_NODES; ++index)
        push_stack_free_list(&head_stack_free_list, &StaticPoolStack[index]);
}

/* Stack related functions */
void push(node_stack** head, void *t)
{
    node_stack* temp = pop_stack_free_list(&head_stack_free_list);
    assert(temp);
    temp->data  = t;
    temp->next = (*head);
    *head= temp;
}
_Bool isEmpty(node_stack *head)
{
    return (head == NULL)? 1 : 0;
}
void *pop(node_stack** head)
{
    void *res;
    node_stack *top;

    assert(!isEmpty(*head));
    top = *head;
    res = top->data;
    *head = top->next;
    push_stack_free_list(&head_stack_free_list, top);
    return res;
}

void* top(node_stack* head)
{
    return head->data;
}

/* binary tree data structure */
struct bin_tree {
    int data;
    struct bin_tree * right, * left;
};
typedef struct bin_tree node_tree;

/* Auxiliary memory tree allocation utilities */
static node_tree StaticPoolTree[MAX_NUMBER_OF_NODES];
static node_tree* head_tree_free_list;

void push_tree_free_list(node_tree ** head, node_tree * new_node)
{
    new_node->data = 0;
    new_node->left = *head;
    new_node->right = 0;
    *head = new_node;
}
node_tree* pop_tree_free_list(node_tree ** head)
{
    node_tree* retval = 0;
    node_tree * next_node = NULL;

    if (*head == NULL)
        return NULL;

    next_node = (*head)->left;
    retval = *head;
    *head = next_node;

    return retval;
}
void init_tree_free_list()
{
    int index;
    for(index=0; index < MAX_NUMBER_OF_NODES; ++index)
        push_tree_free_list(&head_tree_free_list, &StaticPoolTree[index]);
}


/* binary tree functions */
void insert(node_tree ** tree, int val)
{
    node_tree *temp = NULL;
    if(!(*tree))
    {
        temp = pop_tree_free_list(&head_tree_free_list);
        assert(temp);
        temp->left = temp->right = NULL;
        temp->data = val;
        *tree = temp;
        return;
    }
    if(val < (*tree)->data)
    {
        insert(&(*tree)->left, val);
    }
    else if(val > (*tree)->data)
    {
        insert(&(*tree)->right, val);
    }
}

void print_preorder(node_tree * root)
{
    if (root)
    {
        node_tree *current;
        node_stack *s = NULL;
        push(&s, root);

        while (!isEmpty(s))
        {
            current = pop(&s);
            printf ("%d\n", current->data);

            if (current->right)
                push(&s, current->right);
            if (current->left)
                push(&s, current->left);
        }
    }
}

/* Iterative function for inorder binary tree print */
void print_inorder(node_tree *root)
{
    node_tree *current = root;
    node_stack *s = NULL;
    _Bool done = 0;

    while (!done)
    {
        if(current !=  NULL)
        {
            push(&s, current);
            current = current->left;
        }
        else
        {
            if (!isEmpty(s))
            {
                current = pop(&s);
                printf("%d\n", current->data);
                current = current->right;
            }
            else
                done = 1;
        }
    }
}


void print_postorder(node_tree * root)
{
    if (root)
    {
        node_tree *prev=NULL;
        node_stack *s = NULL;
        push(&s, root);

        while (!isEmpty(s)) {
            node_tree *curr = top(s);
            if (!prev || prev->left == curr || prev->right == curr) {
                if (curr->left)
                    push(&s, curr->left);
                else if (curr->right)
                    push(&s, curr->right);
            } else if (curr->left == prev) {
                if (curr->right)
                    push(&s, curr->right);
            } else {
                printf("%d\n", curr->data);
                pop(&s);
            }
            prev = curr;
        }
    }
}

void deltree(node_tree * root)
{
    if (root)
    {
        node_tree *prev=NULL;
        node_stack *s = NULL;
        push(&s, root);

        while (!isEmpty(s)) {
            node_tree *curr = top(s);
            if (!prev || prev->left == curr || prev->right == curr) {
                if (curr->left)
                    push(&s, curr->left);
                else if (curr->right)
                    push(&s, curr->right);
            } else if (curr->left == prev) {
                if (curr->right)
                    push(&s, curr->right);
            } else {
                push_tree_free_list(&head_tree_free_list, curr);
                pop(&s);
            }
            prev = curr;
        }
    }
}

node_tree* __attribute__ ((noinline)) search(node_tree * tree, int val)
{
    if(tree == NULL|| tree->data == val)
        return tree;

    if (tree->data < val)
       return search(tree->right, val);
    else 
       return search(tree->left, val);
}

int main()
{
    node_tree *root;
    node_tree *tmp;
    //int i;
    init_tree_free_list();
    init_stack_free_list();

    root = NULL;
    /* Inserting nodes into tree */
    insert(&root, 9);
    insert(&root, 4);
    insert(&root, 15);
    insert(&root, 6);
    insert(&root, 12);
    insert(&root, 17);
    insert(&root, 2);

    /* Printing nodes of tree */
    printf("Pre Order Display\n");
    print_preorder(root);

    printf("In Order Display\n");
    print_inorder(root);

    printf("Post Order Display\n");
    print_postorder(root);

    /* Search node into tree */
#ifdef BAMBU_PROFILING
  __builtin_bambu_time_start();
#endif
    tmp = search(root, 4);
#ifdef BAMBU_PROFILING
  __builtin_bambu_time_stop();
#endif
    if (tmp)
    {
        printf("Searched node=%d\n", tmp->data);
    }
    else
    {
        printf("Data Not found in tree.\n");
    }

    /* Deleting all nodes of tree */
    deltree(root);
    return 0;
}
