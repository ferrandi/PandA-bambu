# ----------------------------------------------------------------------
# XML Query Helper — Usage
#
# Requires: `xmlq` script in PATH.
# See: ./xmlq --help
#
# Example usage:
#
#   # Path to the XML file
#   set xml "/path/to/bambu_results.xml"
#
#   # Get a single attribute or element value (first match)
#   set version [lindex [xmlget $xml /application@version] 0]
#
#   # Get all matching values as a list
#   set outputs [xmlget $xml /application/outputs/file]
#
#   # Dump the entire XML once into a dict: path -> list of values
#   set data [xmldump $xml]
#   puts "Top module: [lindex [dict get $data /application/top_module@name] 0]"
#   puts "Target vendor: [lindex [dict get $data /application/target@vendor] 0]"
#
# Functions provided:
#   xmlget  FILE PATH   → list of values for the given path
#   xmldump FILE        → dict mapping full paths to lists of values
#
# Notes:
#   • PATH syntax matches `xmlq` output: `/root/child` or `/root/child@attr`
#   • For multiple repeated nodes, you always get a list of values.
#   • If no value exists, returns an empty list.
#
# ----------------------------------------------------------------------

# Returns a list of values (one element per matching node/attr).
proc xmlget {file path} {
   set out [exec -- xmlq $file $path]
   # xmlq prints one value per line; trim trailing newline and split
   set out [string trim $out]
   if {$out eq {}} { return {} }
   return [split $out "\n"]
}

# Returns a dict: path -> list of values (for all nodes/attrs)
# Useful when you want many fields without multiple execs.
proc xmldump {file} {
   set out [exec -- xmlq $file]
   set d {}
   foreach line [split [string trim $out] "\n"] {
      # Each line is "/path value"; split on the first space only
      set sp [string first " " $line]
      if {$sp < 0} {
         # path with empty value
         set key $line
         set val {}
      } else {
         set key [string range $line 0        [expr {$sp-1}]]
         set val [string range $line [expr {$sp+1}] end]
      }
      # Some paths may repeat; accumulate values into a list
      dict lappend d $key $val
   }
   return $d
}
