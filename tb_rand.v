
//
// testbench for arbitrary xmas networks
// configured for specific design through command line params in makefile
//
//`define SIMULATION_ONLY
//

`timescale 1ns/1ps 

`ifndef SIM_CYCLES
 `define SIM_CYCLES 1000
`endif

module tb();
   
   reg 				     clk;
   reg 				     reset;
   reg [`NUM_ORACLES-1:0] 	     oracles;

   nut inst( 
		 .clk(clk), 
		 .reset(reset),
		 .oracles(oracles)
		);
      
   always    
     #0.5 clk = ~clk;
   
   always @(posedge clk)
     if (reset)
       oracles <= 0;
     else
       oracles <= $random; // set PIs random for simulation
   
   
   initial 
     begin
	
	$display ("\nstarting tb simulation at %d", $time);
	
	// print vcd for viewing in a waveform viewer (e.g. gtkwave -f dump.vcd )
	$dumpfile( "dump.vcd" );  
	$dumpvars(0); 
	
    	reset = 1;
	clk   = 1;
	
	#1;
	reset        = 0;
	
	#`SIM_CYCLES;
	$display ("\ntb simulation ends normally at %7d \n", $time);
	$finish;  		
     end

endmodule
