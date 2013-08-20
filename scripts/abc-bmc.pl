#!/usr/bin/env perl


use Carp;
use strict;
use Data::Dumper::Simple;
$Data::Dumper::Sortkeys = 1;
use Getopt::Long;

require "abc_utils.pl";

my $File = "dump.v";
my $Max_Frames = 100;
my $Timeout = 20;
GetOptions (
	    "file=s"        => \$File, 
	    "max_frames=i"  => \$Max_Frames, 
	    "timeout=i"     => \$Timeout, 
	   ) or print "ERROR: bad input options"; 

print "\n";
print "="x70,"\n";; 
print "| ",$0,"\n";
print "| ","file:           $File\n";
print "| ","max_frames:     $Max_Frames\n";
print "| ","timeout:        $Timeout\n";
print "="x70; 
print "\n";


my $aig = convert_to_aig($File); 

my $r = abc_bmc({  "INPUT_FILE" => $aig, 
		   "MAX_FRAMES" => $Max_Frames,
		   "TIMEOUT"    => $Timeout   });

print Dumper($r);

# if input is a verilog file, and cex is found, simulate it to get a vcd trace
if ($aig ne $File) {
     if ($r->{INVALID}) {
	  my $foo = system('cex2v.pl');
	  croak unless ($foo eq 0);
	  system("iverilog-sim.sh $File tb_cex.v");
     } else {
	  print "no cex to simulate\n";
     }
}

exit(0);
