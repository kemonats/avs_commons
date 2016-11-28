#ifndef AVS_COMMONS_RBTREE_INCLUDE_PUBLIC_COMMONS_RBTREE_H
#define AVS_COMMONS_RBTREE_INCLUDE_PUBLIC_COMMONS_RBTREE_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <avsystem/commons/defs.h>

/**
 * RB-tree element comparator.
 *
 * @param a    First element. Note that the comparator must be aware of actual
 *             type of compared elements.
 * @param b    Second element.
 *
 * @returns:
 * - a negative value if @p a < @p b,
 * - 0 if @p a == @p b,
 * - a positive value if @p a > @p b.
 *
 * WARNING: The comparator MUST establish a total ordering of RB-tree elements.
 * Moreover, element components used to establish an ordering MUST be immutable
 * for the whole lifetime of an RB-tree, otherwise the behavior of operations on
 * the tree is undefined.
 *
 * NOTE: Since a comparator may use only a fragment of the RB-tree element, it
 * is possible to implement a map (dictionary) by storing a { key; value }
 * struct in each tree element and comparing them by key.
 */
typedef int avs_rbtree_element_comparator_t(const void *a,
                                            const void *b);

/**
 * RB-tree element deleter. May be passed to @ref AVS_RBTREE_DELETE to perform
 * additional cleanup for each deleted element.
 *
 * NOTE: the deleter MUST NOT call <c>free()</c> on the @p elem itself.
 *
 * @param elem Pointer to the element to perform cleanup on.
 */
typedef void avs_rbtree_element_deleter_t(void *elem);

/** RB-tree type alias.  */
#define AVS_RBTREE(type) type**
/** RB element type alias. */
#define AVS_RBTREE_ELEM(type) type*

#define _AVS_RB_TYPECHECK(tree_type, elem_type) \
    ((void)(**(tree_type) < *(elem_type)))

/**
 * Create an RB-tree with elements of given @p type.
 *
 * @param type Type of elements stored in the tree nodes.
 * @param cmp  Pointer to a function that compares two elements.
 *             See @ref avs_rbtree_element_comparator_t .
 *
 * @returns Created RB-tree object on success, NULL in case of error.
 */
#define AVS_RBTREE_NEW(type, cmp) ((AVS_RBTREE(type))avs_rbtree_new__(cmp))

/**
 * Releases given RB-tree and all its nodes.
 *
 * @param tree_ptr Pointer to the RB-tree object to destroy. *tree_ptr is set to
 *                 NULL after the cleanup is done.
 * @param deleter  Cleanup callback, called before deleting each tree element.
 *                 May be NULL if no additional cleanup is required.
 */
#define AVS_RBTREE_DELETE(tree_ptr, deleter) \
    avs_rbtree_delete__((AVS_RBTREE(void)*)(tree_ptr), (deleter))

/**
 * @param tree RB-tree object to operate on.
 *
 * @returns Total number of elements stored in the tree.
 */
#define AVS_RBTREE_SIZE(tree) avs_rbtree_size__((const AVS_RBTREE(void))tree)

/**
 * Creates an arbitrarily-sized, detached RB-tree element.
 *
 * Example:
 * @code
 * struct string {
 *     size_t size;
 *     char c_str[]; // C99 flexible array member
 * };
 *
 * const size_t STRING_CAPACITY = 64;
 *
 * AVS_RBTREE_ELEM(struct string) node = (AVS_RBTREE_ELEM(struct string))
 *     AVS_RBTREE_ELEM_NEW_BUFFER(sizeof(struct string) + STRING_CAPACITY);
 * // allocated string is able to hold STRING_CAPACITY characters in its c_str
 * // member array
 * @endcode
 *
 * NOTE: the in-memory representation of a node is as follows:
 * <pre>
 *                                     element pointers point here
 *                                     |
 *                                     v
 * +========+========+========+========+======================================+
 * |  color | parent |  left  |  right | element value                        |
 * +========+========+========+========+======================================+
 *  { sizeof(int) + 3 * sizeof(void)  } { -------- arbitrary size ---------- }
 *  {           (+ padding)           }
 *
 * </pre>
 *
 * @param size Number of bytes to allocate for the element content.
 *
 * @returns Pointer to created element on success, NULL in case of error.
 */
#define AVS_RBTREE_ELEM_NEW_BUFFER(size) avs_rbtree_elem_new_buffer__(size)

/**
 * Creates a detached RB-tree element large enough to hold a value of
 * given @p type.
 *
 * @param type Desired element type.
 *
 * @returns Pointer to created element cast to @p type * on success,
 *          NULL in case of error.
 */
#define AVS_RBTREE_ELEM_NEW(type) \
    ((type*)AVS_RBTREE_ELEM_NEW_BUFFER(sizeof(type)))

/**
 * Frees memory associated with given detached RB-tree element.
 *
 * NOTE: when passed @p elem is attached to some tree, the behavior
 * is undefined.
 *
 * @param elem_ptr Pointer to element to free. *elem is set to NULL after
 *                 cleanup.
 */
#define AVS_RBTREE_ELEM_DELETE_DETACHED(elem) \
    avs_rbtree_elem_delete__((AVS_RBTREE_ELEM(void)*)elem, NULL)

/**
 * Inserts a detached @p elem into given @p tree, if an element
 * equivalent to @p elem (wrt. @ref avs_rbtree_element_comparator_t
 * of @p tree) does not yet exist in the tree.
 *
 * NOTE: when passed @p elem is attached to some tree, the behavior
 * is undefined.
 *
 * @param tree Tree to insert element into.
 * @param elem Element to insert.
 *
 * @returns:
 * - @p elem on success,
 * - a pointer to the equivalent element if one already existed in the tree.
 */
#define AVS_RBTREE_INSERT(tree, elem) \
    (_AVS_RB_TYPECHECK(tree, elem), \
     (AVS_TYPEOF_PTR(elem))avs_rbtree_attach__((AVS_RBTREE(void))(tree), (elem)))

/**
 * Detaches given @p elem from @p tree. Does not free @p elem.
 *
 * NOTE: when passed @p elem is not attached to @p tree, the behavior
 * is undefined.
 *
 * @param tree Tree to remove element from.
 * @param elem Element to remove.
 *
 * @returns Detached @p elem.
 */
#define AVS_RBTREE_DETACH(tree, elem) \
    (_AVS_RB_TYPECHECK(tree, elem), \
     avs_rbtree_detach__((AVS_RBTREE(void))(tree), (elem)))

/**
 * Deletes a @p elem attached to @p tree by detaching it from @p tree,
 * performing cleanup using @p deleter and releasing allocated memory.
 *
 * NOTE: when passed @p elem is not attached to @p tree, the behavior
 * is undefined.
 *
 * @param tree     Tree to remove element from.
 * @param elem_ptr Pointer to the element to remove. On success, *elem_ptr
 *                 will be set to NULL.
 * @param deleter  Pointer to a cleanup function to call before releasing
 *                 @p elem. May be NULL if no extra cleanup is necessary.
 */
#define AVS_RBTREE_DELETE_ELEM(tree, elem_ptr, deleter) \
    do { \
        AVS_RBTREE_ELEM(void) found = AVS_RBTREE_DETACH((tree), *(elem_ptr)); \
        assert(found && "node not found in the tree"); \
        avs_rbtree_elem_delete__((AVS_RBTREE_ELEM(void)*)(elem_ptr), \
                                 (deleter)); \
    } while (0)

/**
 * Finds an element with value given by @p val_ptr in @p tree.
 *
 * @param tree    Tree to search in.
 * @param val_ptr Pointer to a node value to search for.
 *                NOTE: this does not need to be an AVS_RBTREE_ELEM object.
 *
 * @returns Found attached element pointer on success, NULL if the @p tree does
 *          not contain such element.
 */
#define AVS_RBTREE_FIND(tree, val_ptr) \
    (_AVS_RB_TYPECHECK(tree, val_ptr), \
     ((AVS_TYPEOF_PTR(val_ptr)) \
      avs_rbtree_find__((const AVS_RBTREE(void))tree, val_ptr)))

/**
 * @returns:
 * - @p elem successor in order defined by
 *   @ref avs_rbtree_element_comparator_t of an RB-tree object @p elem is
 *   attached to.
 * - NULL if there is no successor or the node is detached.
 */
#define AVS_RBTREE_ELEM_NEXT(elem) \
    ((AVS_TYPEOF_PTR(elem))avs_rbtree_elem_next__(elem))

/**
 * @returns:
 * - @p elem predecessor in order defined by
 *   @ref avs_rbtree_element_comparator_t of an RB-tree object @p elem is
 *   attached to.
 * - NULL if there is no predecessor or the node is detached.
 */
#define AVS_RBTREE_ELEM_PREV(elem) \
    ((AVS_TYPEOF_PTR(elem))avs_rbtree_elem_prev__(elem))

/**
 * @returns the first element in @p tree (in order defined by
 *          @ref avs_rbtree_element_comparator_t of @p tree).
 */
#define AVS_RBTREE_FIRST(tree) \
    ((AVS_TYPEOF_PTR(*tree))avs_rbtree_first__((AVS_RBTREE(void))tree))

/**
 * @returns the last element in @p tree (in order defined by
 *          @ref avs_rbtree_element_comparator_t of @p tree).
 */
#define AVS_RBTREE_LAST(tree) \
    ((AVS_TYPEOF_PTR(*tree))avs_rbtree_last__((AVS_RBTREE(void))tree))

/** Convenience macro for forward iteration on elements of @p tree. */
#define AVS_RBTREE_FOREACH(it, tree) \
    for (it = AVS_RBTREE_FIRST(tree); \
            it; \
            it = AVS_RBTREE_ELEM_NEXT(it))

/** Convenience macro for backward iteration on elements of @p tree. */
#define AVS_RBTREE_FOREACH_REVERSE(it, tree) \
    for (it = AVS_RBTREE_LAST(tree); \
            it; \
            it = AVS_RBTREE_ELEM_PREV(it))

/* Internal functions. Use macros defined above instead. */
AVS_RBTREE(void) avs_rbtree_new__(avs_rbtree_element_comparator_t *cmp);
void avs_rbtree_delete__(AVS_RBTREE(void) *tree,
                        avs_rbtree_element_deleter_t *deleter);

size_t avs_rbtree_size__(const AVS_RBTREE(void) tree);
AVS_RBTREE_ELEM(void) avs_rbtree_find__(const AVS_RBTREE(void) tree,
                                        const void *value);
AVS_RBTREE_ELEM(void) avs_rbtree_attach__(AVS_RBTREE(void) tree,
                                          AVS_RBTREE_ELEM(void) node);
AVS_RBTREE_ELEM(void) avs_rbtree_detach__(AVS_RBTREE(void) tree,
                                          AVS_RBTREE_ELEM(void) node);

AVS_RBTREE_ELEM(void) avs_rbtree_first__(AVS_RBTREE(void) tree);
AVS_RBTREE_ELEM(void) avs_rbtree_last__(AVS_RBTREE(void) tree);

AVS_RBTREE_ELEM(void) avs_rbtree_elem_new_buffer__(size_t elem_size);
void avs_rbtree_elem_delete__(AVS_RBTREE_ELEM(void) *node,
                              avs_rbtree_element_deleter_t *deleter);

AVS_RBTREE_ELEM(void) avs_rbtree_elem_next__(AVS_RBTREE_ELEM(void) elem);
AVS_RBTREE_ELEM(void) avs_rbtree_elem_prev__(AVS_RBTREE_ELEM(void) elem);

#endif /* AVS_COMMONS_RBTREE_INCLUDE_PUBLIC_COMMONS_RBTREE_H */
