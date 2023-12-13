struct GTY(()) tree_2_ints
{
   tree key;
   unsigned int value;
   unsigned int has_body;
   unsigned int ann;
};
struct GTY(()) dg_descriptor_tree
{
   /* The VAR_DECL.  */
   tree vd;
   hashval_t hash;
};

extern GTY((if_marked("dg_descriptor_tree_marked_p"),
            param_is(struct dg_descriptor_tree))) htab_t di_local_referenced_var_htab;

extern GTY((if_marked("tree_2_ints_marked_p"), param_is(struct tree_2_ints))) htab_t di_local_nodes_index;

extern GTY(()) unsigned int di_local_index;
