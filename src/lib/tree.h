/*
 * Directory tree build/traverse routines
 * 
 *    Kern Sibbald, June MMII
 *
*/
/*
   Copyright (C) 2002-2004 Kern Sibbald and John Walker

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.

 */

struct s_mem {
   struct s_mem *next;		      /* next buffer */
   int rem;			      /* remaining bytes */
   char *mem;			      /* memory pointer */
   char first[1];		      /* first byte */
};

#define USE_DLIST
#ifdef	USE_DLIST
#define foreach_child(var, list) \
    for((var)=NULL; (*((TREE_NODE **)&(var))=(TREE_NODE*)(list->child.next(var))); )

#define tree_node_has_child(node) \
	((node)->child.size() > 0)

#define first_child(node) \
	((TREE_NODE *)(node->child.first())


/*
 * Keep this node as small as possible because
 *   there is one for each file.
 */
struct s_tree_node {
   /* KEEP sibling as the first member to avoid having to
    *  do initialization of child */
   dlink sibling;
   dlist child;
   char *fname; 		      /* file name */
   int32_t FileIndex;		      /* file index */
   uint32_t JobId;		      /* JobId */
   uint16_t fname_len;		      /* filename length */
   int type: 8; 		      /* node type */
   unsigned int extract: 1;	      /* extract item */
   unsigned int extract_dir: 1;       /* extract dir entry only */
   unsigned int hard_link: 1;	      /* set if have hard link */
   unsigned int soft_link: 1;	      /* set if is soft link */  
   unsigned int inserted: 1;	      /* set when newly inserted */
   struct s_tree_node *parent;
   struct s_tree_node *next;	      /* next hash of FileIndex */
};
typedef struct s_tree_node TREE_NODE;

struct s_tree_root {
   /* KEEP sibling as the first member to avoid having to
    *  do initialization of child */
   dlink sibling;
   dlist child;
   const char *fname; 		      /* file name */
   int32_t FileIndex;		      /* file index */
   uint32_t JobId;		      /* JobId */
   uint16_t fname_len;		      /* filename length */
   unsigned int type: 8;	      /* node type */
   unsigned int extract: 1;	      /* extract item */
   unsigned int extract_dir: 1;       /* extract dir entry only */
   unsigned int have_link: 1;	      /* set if have hard link */
   unsigned int inserted: 1;	      /* set when newly inserted */
   struct s_tree_node *parent;
   struct s_tree_node *next;	      /* next hash of FileIndex */

   /* The above ^^^ must be identical to a TREE_NODE structure */
   struct s_tree_node *first;	      /* first entry in the tree */
   struct s_tree_node *last;	      /* last entry in tree */
   struct s_mem *mem;		      /* tree memory */
   uint32_t total_size; 	      /* total bytes allocated */
   uint32_t blocks;		      /* total mallocs */
   int cached_path_len; 	      /* length of cached path */
   char *cached_path;		      /* cached current path */
   TREE_NODE *cached_parent;	      /* cached parent for above path */
};
typedef struct s_tree_root TREE_ROOT;

#else
#define foreach_child(cld, node) \
	for(cld=(node)->child_; cld; cld=cld->sibling_)

#define tree_node_has_child(node) \
	((node)->child_ != NULL)

#define first_child(node) \
	((node)->child_)


/*
 * Keep this node as small as possible because
 *   there is one for each file.
 */
struct s_tree_node {
   char *fname; 		      /* file name */
   int32_t FileIndex;		      /* file index */
   uint32_t JobId;		      /* JobId */
   uint16_t fname_len;		      /* filename length */
   int type: 8; 		      /* node type */
   unsigned int extract: 1;	      /* extract item */
   unsigned int extract_dir: 1;       /* extract dir entry only */
   unsigned int hard_link: 1;	      /* set if have hard link */
   unsigned int soft_link: 1;	      /* set if is soft link */  
   unsigned int inserted: 1;	      /* set when newly inserted */
   struct s_tree_node *parent;
   struct s_tree_node *sibling_;
   struct s_tree_node *next;	      /* next hash of FileIndex */
   struct s_tree_node *child_;
};
typedef struct s_tree_node TREE_NODE;

struct s_tree_root {
   char *fname; 		      /* file name */
   int32_t FileIndex;		      /* file index */
   uint32_t JobId;		      /* JobId */
   uint16_t fname_len;		      /* filename length */
   unsigned int type: 8;	      /* node type */
   unsigned int extract: 1;	      /* extract item */
   unsigned int extract_dir: 1;       /* extract dir entry only */
   unsigned int have_link: 1;	      /* set if have hard link */
   unsigned int inserted: 1;	      /* set when newly inserted */
   struct s_tree_node *parent;
   struct s_tree_node *sibling_;
   struct s_tree_node *next;	      /* next hash of FileIndex */
   struct s_tree_node *child_;

   /* The above ^^^ must be identical to a TREE_NODE structure */
   struct s_tree_node *first;	      /* first entry in the tree */
   struct s_tree_node *last;	      /* last entry in tree */
   struct s_mem *mem;		      /* tree memory */
   uint32_t total_size; 	      /* total bytes allocated */
   uint32_t blocks;		      /* total mallocs */
   int cached_path_len; 	      /* length of cached path */
   char *cached_path;		      /* cached current path */
   TREE_NODE *cached_parent;	      /* cached parent for above path */
};
typedef struct s_tree_root TREE_ROOT;
#endif

/* type values */
#define TN_ROOT    1		      /* root node */
#define TN_NEWDIR  2		      /* created directory to fill path */
#define TN_DIR	   3		      /* directory entry */
#define TN_DIR_NLS 4		      /* directory -- no leading slash -- win32 */
#define TN_FILE    5		      /* file entry */

/* External interface */
TREE_ROOT *new_tree(int count);
TREE_NODE *insert_tree_node(char *path, char *fname, int type,
			    TREE_ROOT *root, TREE_NODE *parent);
TREE_NODE *make_tree_path(char *path, TREE_ROOT *root);
TREE_NODE *tree_cwd(char *path, TREE_ROOT *root, TREE_NODE *node);
TREE_NODE *tree_relcwd(char *path, TREE_ROOT *root, TREE_NODE *node);
void free_tree(TREE_ROOT *root);
int tree_getpath(TREE_NODE *node, char *buf, int buf_size);

/*
 * Use the following for traversing the whole tree. It will be
 *   traversed in the order the entries were inserted into the
 *   tree.	  
 */
#ifdef SLOW_WAY
TREE_NODE *first_tree_node(TREE_ROOT *root);
TREE_NODE *next_tree_node(TREE_NODE *node);
#else
  #define first_tree_node(r) (r)->first
  #define next_tree_node(n)  (n)->next
#endif
