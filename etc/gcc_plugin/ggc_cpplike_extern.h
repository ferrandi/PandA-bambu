struct GTY((for_user)) dg_descriptor_tree
{
   /* The VAR_DECL.  */
   tree vd;
   hashval_t hash;
};

struct GTY((for_user)) tree_2_ints
{
   tree key;
   unsigned int value;
   unsigned int has_body;
   unsigned int ann;
};

#if(__GNUC__ > 5)
struct t2is_hasher : ggc_ptr_hash<tree_2_ints>
{
   /* Hash a tree_2_ints* .  */
   static hashval_t hash(tree_2_ints* item)
   {
      return htab_hash_pointer(item->key);
   }

   /* Return true if the tree_2_ints* are equal.  */
   static bool equal(tree_2_ints* a, tree_2_ints* b)
   {
      return a->key == b->key && a->ann == b->ann;
   }
};
#else
struct t2is_hasher : ggc_hasher<tree_2_ints*>
{
   /* Hash a tree_2_ints* .  */
   static hashval_t hash(tree_2_ints* item)
   {
      return htab_hash_pointer(item->key);
   }

   /* Return true if the tree_2_ints* are equal.  */
   static bool equal(tree_2_ints* a, tree_2_ints* b)
   {
      return a->key == b->key && a->ann == b->ann;
   }
};
#endif

#if(__GNUC__ > 5)
struct dg_hasher : ggc_ptr_hash<dg_descriptor_tree>
#else
struct dg_hasher : ggc_hasher<dg_descriptor_tree*>
#endif
{
   /* Hash a tree in a uid_decl_map.  */
   static hashval_t hash(dg_descriptor_tree* item)
   {
      return item->hash;
   }

   /* Return true if the DECL_UID in both trees are equal.  */
   static bool equal(dg_descriptor_tree* a, dg_descriptor_tree* b)
   {
      return a->hash == b->hash;
   }
};

extern GTY(()) hash_table<t2is_hasher>* di_local_nodes_index;
extern GTY(()) unsigned int di_local_index;
extern GTY(()) hash_table<dg_hasher>* di_local_referenced_var_htab;
