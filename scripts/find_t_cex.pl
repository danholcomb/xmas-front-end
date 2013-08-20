#!/usr/bin/env perl

use Carp;
use strict;
use Data::Dumper::Simple;
$Data::Dumper::Sortkeys = 1;
use Getopt::Long;

require "abc_utils.pl";

my $Exp_Name = "unnamed_t_cex";
my $Param_Name = "t_bound";
my $T_Min  = 0;
my $T_Max = 30;
my $Timeout = 20;
my $Max_Frames = 100;

# which assertions to turn on off. E.g. can override the default to
# find the age bound for single slots.
#my $Build_Args = ' --enable_tag phig_x ';
my $Build_Args = ' --enable_tag phig_x ';


GetOptions (
	    "exp_name=s"      => \$Exp_Name, 
	    "param_name=s"    => \$Param_Name, 
	    "t_min=i"         => \$T_Min, 
	    "t_max=i"         => \$T_Max, 
	    "timeout=i"       => \$Timeout, 
	    "max_frames=i"    => \$Max_Frames, 
	    "build_args=s"    => \$Build_Args, 
	   ) or print "ERROR: bad input options"; 
# print Dumper($Build_Args);
# exit;

my $R = find_t_cex({ "EXP_NAME"     => $Exp_Name,
		     "PARAM_NAME"   => $Param_Name,
		     "T_MIN"        => $T_Min,
		     "T_MAX"        => $T_Max,
		     "TIMEOUT"      => $Timeout,
		     "MAX_FRAMES"   => $Max_Frames,
		     "BUILD_ARGS"   => $Build_Args
		   });

print Dumper($R);
exit($R); #return tcex to use when chaining tests where next range starts at this one



