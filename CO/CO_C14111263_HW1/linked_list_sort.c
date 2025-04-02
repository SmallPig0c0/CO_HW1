#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int data;
    struct Node *next;
} Node;

// 使用 RISC-V 組合語言拆分鏈結串列
void splitList(Node *head, Node **firstHalf, Node **secondHalf) {
    asm volatile(
        "mv t0, %2\n"             // t0 = head
        "beqz t0, 3f\n"           // if head is NULL, return immediately

        "mv t1, t0\n"             // t1 = slow (slow pointer)
        "mv t2, t0\n"             // t2 = fast (fast pointer)
        "mv t3, zero\n"           // t3 = prev (NULL)

        "1:\n"
        "ld t4, 8(t2)\n"          // t4 = fast->next
        "beqz t4, 2f\n"           // if fast->next == NULL, stop
        "ld t5, 8(t4)\n"          // t5 = fast->next->next
        "beqz t5, 2f\n"           // if fast->next->next == NULL, stop

        "mv t3, t1\n"            // prev = slow
        "ld t1, 8(t1)\n"         // slow = slow->next
        "mv t2, t5\n"            // fast = fast->next->next

        "j 1b\n"                 // continue loop

        "2:\n"
        "beqz t3, 3f\n"          // if prev == NULL, don't break
        "sd zero, 8(t3)\n"       // prev->next = NULL (split list)

        "3:\n"
        "mv %0, t0\n"            // firstHalf = head
        "mv %1, t1\n"            // secondHalf = slow

        : "=r"(*firstHalf), "=r"(*secondHalf)
        : "r"(head)
        : "t0", "t1", "t2", "t3", "t4", "t5", "memory"
    );
} //correct

// Merge two sorted linked lists
Node *mergeSortedLists(Node *a, Node *b) {
    Node *result = NULL;
    Node *tail = NULL;
    
    asm volatile(
        // 初始化
        "mv t0, %1\n"            // t0 = a (指向第一個鏈表)
        "mv t1, %2\n"            // t1 = b (指向第二個鏈表)
        "mv t2, zero\n"          // t2 = result (初始化結果鏈表為 NULL)
        "mv t3, zero\n"          // t3 = tail (初始化尾部為 NULL)

        "1:\n"                    // 循環開始
        "beqz t0, 4f\n"           // 如果 a == NULL，跳到處理 b
        "beqz t1, 4f\n"           // 如果 b == NULL，跳到處理 a

        "lw t4, 0(t0)\n"          // t4 = a->data
        "lw t5, 0(t1)\n"          // t5 = b->data
        "blt t4, t5, 2f\n"        // 如果 a->data < b->data，跳到處理 a

        // b 比 a 小，插入 b 到結果中
        "ld t6, 8(t1)\n"          // t6 = b->next
        "beqz t2, 3f\n"           // 如果 result == NULL, 設置 result = b
        "sd t1, 8(t3)\n"          // tail->next = b (更新尾部指針)
        "j 3f\n"

        "2:\n"                    // a 比 b 小，插入 a 到結果中
        "ld t6, 8(t0)\n"          // t6 = a->next
        "beqz t2, 3f\n"           // 如果 result == NULL, 設置 result = a
        "sd t0, 8(t3)\n"          // tail->next = a (更新尾部指針)

        "3:\n"
        "mv t3, t0\n"             // tail = a 或 b
        "mv t0, t6\n"             // a = a->next 或 b = b->next
        "j 1b\n"                  // 跳回循環

        // 當其中一條鏈結串列處理完時，直接接上另一條
        "4:\n"
        "beqz t0, 5f\n"           // 如果 a == NULL，跳到處理 b
        "sd t0, 8(t3)\n"          // tail->next = a
        "j 6f\n"

        "5:\n"
        "beqz t1, 6f\n"           // 如果 b == NULL，跳到結束
        "sd t1, 8(t3)\n"          // tail->next = b

        "6:\n"
        "mv %0, t2\n"             // 返回結果鏈表頭部

        : "=r"(result)            // 輸出變數 (用 result 取代 a0)
        : "r"(a), "r"(b)          // 輸入變數
        : "t0", "t1", "t2", "t3", "t4", "t5", "t6", "memory"  // 受影響的暫存器
    );
    
    return result;
}

// Merge Sort function for linked list
Node *mergeSort(Node *head) {
    if (!head || !head->next) {
        return head;  // Return directly if there is only one node
    }

    Node *firstHalf, *secondHalf;
    
    // Split the list into two sublists
    splitList(head, &firstHalf, &secondHalf);

    // Recursively sort the left half
    firstHalf = mergeSort(firstHalf);

    // Recursively sort the right half
    secondHalf = mergeSort(secondHalf);

    // Merge the sorted sublists
    return mergeSortedLists(firstHalf, secondHalf);
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    FILE *input = fopen(argv[1], "r");
    if (!input) {
        fprintf(stderr, "Error opening file: %s\n", argv[1]);
        return 1;
    }

    int list_size;
    fscanf(input, "%d", &list_size);

    Node *head = (list_size > 0) ? (Node*)malloc(sizeof(Node)) : NULL;
    Node *cur = head;

    for (int i = 0; i < list_size; i++) {
        fscanf(input, "%d", &cur->data);
        if (i + 1 < list_size) 
            cur->next = (Node*)malloc(sizeof(Node));
        cur = cur->next;
        
    }
    fclose(input);

    // Linked list sort
    head = mergeSort(head);
    
    cur = head;
    while (cur) {
        printf("%d ", cur->data);
        asm volatile(
            "ld t0, 8(%0)\n"          // 正確的偏移量: t0 = cur->next (加載 cur 的下一個節點)
            "mv %0, t0\n"             // 更新 cur，指向下一個節點
            : "=r"(cur)               // 更新 cur
            : "0"(cur)                // 輸入 cur
            : "t0"                    // 使用的暫存器
        );
    }
    printf("\n");
    return 0;
}
