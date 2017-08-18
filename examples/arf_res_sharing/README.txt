In this directory the impact of resource sharing on multipliers for the arf benchmark is considered. Two sets of scripts are provided: constrained and non-constrained based synthesis scripts.
The devices considered are all the ones supported by bambu.
In all the synthesis performed, the WB4 interface has been used to avoid issues with the high number of IO pins required by the arf function when synthesized alone.
Basically, adding a constraints on the number of multipliers used just requires to pass to bambu a xml file structured in this way:
<?xml version="1.0"?>
<constraints>
   <HLS_constraints>
      <tech_constraints fu_name="mult_expr_FU" fu_library="STD_FU" n="1"/>
   </HLS_constraints>
</constraints>

