#!/usr/bin/env perl

use Carp;
use strict;
use Data::Dumper::Simple;
$Data::Dumper::Sortkeys = 1;
use Getopt::Long;

require "abc_utils.pl";

my $File = "dump.v";
my $Max_Frames = 40;
my $Timeout = 100;
my $Exp_Name = "unnamed_abc_kind";

GetOptions (
	    "file=s"       => \$File, 
	    "exp_name=s"   => \$Exp_Name, 
	    "max_frames=i" => \$Max_Frames, 
	    "timeout=i"    => \$Timeout, 
	   ) or print "ERROR: bad input options"; 

print "\n";
print "="x70,"\n";; 
print "| ",$0,"\n";
print "| ","file:              $File\n";
print "| ","max_frames:        $Max_Frames\n";
print "| ","timeout:           $Timeout\n";
print "| ","exp_name:          $Exp_Name\n";
print "="x70; 
print "\n";

my $r = abc_kind({  "INPUT_FILE" => $File, 
#		    "K"          => $Frames,
		    "MAX_FRAMES" => $Max_Frames,
		    "TIMEOUT"    => $Timeout,
		    "EXP_NAME"   => $Exp_Name    });

print Dumper($r);

