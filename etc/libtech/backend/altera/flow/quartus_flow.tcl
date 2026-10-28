cd [file dirname [info script]]
source ../../utils/xmlq_wrapper.tcl

set bambu_results [xmldump $env(BAMBU_HLS_RESULTS)]

load_package flow
project_open [dict get $bambu_results /application/top_module@name]
if { ! [dict get $bambu_results /application/target@connect_iob] } {
   execute_module -tool map
   set name_ids [get_names -filter * -node_type pin]
   foreach_in_collection name_id $name_ids {
      set pin_name [get_name_info -info full_path $name_id]
      post_message "Making VIRTUAL_PIN assignment to $pin_name"
      set_instance_assignment -to $pin_name -name VIRTUAL_PIN ON
   }
   export_assignments
}
execute_flow -compile
project_close
