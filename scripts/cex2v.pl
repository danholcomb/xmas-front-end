#!/usr/bin/env perl


# use local::lib "/home/holcomb/Perl_Modules"; #installs modules without sudo
use Carp;
# use Data::Dumper::Simple;
# $Data::Dumper::Sortkeys = 1;

use Getopt::Long;
my $Fname_Cex = "foo.cex";
my $Fname_Pio = "dump.abc";
my $Fname_Tb  = "tb_cex.v";
$result = GetOptions (
		      "cex=s" => \$Fname_Cex, # string
		      "pio=s" => \$Fname_Pio, # string
		     ); 

my $dump = readFileScalar($Fname_Pio);

system('rm tb_cex.v'); #prevent old cex from messing things up

# read the POs and find out which was asserted
if ($dump =~ /Primary outputs \((\d+)\):\s*(.*)\n/) {
     $Num_PO = $1;
     @PO = split(' ',$2);
} else {
     croak "\n\ncannot find POs";
}

if ($dump =~ /Output (\d+) asserted in frame (\d+)/) {
     $Asserted_PO = $1;
     $Frame = $2;
} else {
     croak "no output asserted in bmc output";
}

print "\n";
print "="x70,"\n";; 
print "| ",$0,"\n";
print "| ","Reading PO Names from         $Fname_Pio \n";
print "| ","Number of POs in file =       $Num_PO\n";
print "| ","Reading ABC Cex from          $Fname_Cex \n";
print "| ","idx of Asserted PO in Cex =   $Asserted_PO\n";
print "| ","Name of Asserted PO =         $PO[$Asserted_PO]\n";
print "| ","PO Asserted in Frame =        $Frame\n";
print "| ","Writing Cex Testbench to      $Fname_Tb\n";
print "="x70; 
print "\n";

#print Dumper($Num_PO, $Asserted_PO, $PO[$Asserted_PO],$Frame);

#my $cex = readFileScalar("foo.cex");
my $cex = readFileScalar($Fname_Cex);
my @cex_assignments = split(' ',$cex);
my @cex_assignments = grep {$_ =~ /oracles/} @cex_assignments; #skip the initial state junk
#print Dumper(@cex_assignments);


# vectorize the cex
foreach my $c (@cex_assignments) {
     if ($c =~ /\/.*\/(oracles\[\d+\])@(\d+)=([01])/) {
	  my ($sig, $cycle, $val) = ($1,$2,$3);
	  $cycle_assignments[$cycle] .= "$sig=1'b$val;  ";
	  #print Dumper($sig,$cycle,$val);
     } else {
	  croak "something wrong";
     }
}


#make a new tb using the cex;
my $tb_cex = '
`timescale 1ns/1ps 
module tb();
   
   reg 				     clk;
   reg 				     reset;
   reg [`NUM_ORACLES-1:0] oracles;
   
   nut inst( 	 .clk(clk), 
		 .reset(reset),
		 .oracles(oracles)	);
      
   always    
     #0.5 clk = ~clk;
      
   initial 
     begin	
	$display ("num oracles: %d", `NUM_ORACLES);

	// print vcd for viewing in a waveform viewer (e.g. gtkwave -f dump.vcd )
	$dumpfile( "dump.vcd" );  
	$dumpvars(0); 
	
    	reset = 1;
	clk   = 1;	
';
$tb_cex .= "$cycle_assignments[$_] #1// cex cycle $_ \n" foreach (0);
$tb_cex .= "reset = 0;\n";
$tb_cex .= "$cycle_assignments[$_] #1// cex cycle $_ \n" foreach (1..$#cycle_assignments);
$tb_cex .= '	       	
	#20;
	$display ("\ntestbench ends cex sim at %7d \n", $time);
        $finish;  		
     end
endmodule
';
writeFile($Fname_Tb, $tb_cex);
exit(0); #success

sub writeFile {
     my ( $f, @data ) = @_;
     @data = () unless @data;
     open F, "> $f" or die "Can't open $f : $!";
     print F @data;
     close F;
}


sub readFileScalar  {
     my ( $f ) = @_;
     open F, "< $f" or die "Can't open $f : $!";
     my @f = <F>;		close F;
     return join("",@f);
}
