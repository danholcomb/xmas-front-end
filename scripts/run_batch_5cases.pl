#!/usr/bin/env perl

use Carp;
use strict;
use Data::Dumper::Simple;
$Data::Dumper::Sortkeys = 1;
use Getopt::Long;

require "abc_utils.pl";

my $Timeout = 120;
my $Exp_Name = "unnamed_batch_experiment";
my $Build_Args = ' ';
my $No_Bmc = '';		#skip the bmc and the tight bound checking

GetOptions (
	    "exp_name=s"    => \$Exp_Name, 
	    "timeout=i"     => \$Timeout, 
	    "build_args=s"  => \$Build_Args, 
	    "no_bmc"        => \$No_Bmc, 
	   ) or print "ERROR: bad input options"; 

print "\n";
print "="x70,"\n";; 
print "| ",$0,"\n";
print "| ","exp_name:             $Exp_Name\n";
print "| ","timeout:              $Timeout\n";
print "| ","build_args:           $Build_Args\n";
print "| ","no_bmc:               $No_Bmc\n";
print "="x70; 
print "\n";

my $Tight = "";
unless ($No_Bmc) {
     # may just be a timeout and not really bound
     my $tcex = find_t_cex({ "EXP_NAME"     => $Exp_Name.".tcex",
			     "T_MIN"        => 3,
			     "T_MAX"        => 100,
			     "TIMEOUT"      => $Timeout,
			     "MAX_FRAMES"   => 200,
			     "BUILD_ARGS"   => $Build_Args." --enable_tag phig_x --enable_tag psi "
			   });
     print Dumper($tcex);
     $Tight       = " --enable_tag phig_x --t_bound ".($tcex+1)." "; # extra stuff to check tight bound
}

my $Assertions1 = " --enable_tag phig_tl "; #induction cant solve w/o psi
my $Assertions2 = " --enable_tag phig_tl --enable_tag psi ";
my $Assertions3 = " --enable_tag phig_tl --enable_tag psi --enable_tag phil --enable_tag psil "; 



my $experiments = ();

push (@$experiments, { "ENGINE" => "kind" , "BUILD_ARGS" => " $Build_Args $Assertions2 "}); 
push (@$experiments, { "ENGINE" => "kind" , "BUILD_ARGS" => " $Build_Args $Assertions3 "}); 

push (@$experiments, { "ENGINE" => "pdr"  , "BUILD_ARGS" => " $Build_Args $Assertions1 "}); 
push (@$experiments, { "ENGINE" => "pdr"  , "BUILD_ARGS" => " $Build_Args $Assertions2 "}); 
push (@$experiments, { "ENGINE" => "pdr"  , "BUILD_ARGS" => " $Build_Args $Assertions3 "}); 

unless ($No_Bmc) {
     push (@$experiments, { "ENGINE" => "kind" , "BUILD_ARGS" => " $Build_Args $Assertions2 $Tight"}); 
     push (@$experiments, { "ENGINE" => "kind" , "BUILD_ARGS" => " $Build_Args $Assertions3 $Tight"}); 

     push (@$experiments, { "ENGINE" => "pdr"  , "BUILD_ARGS" => " $Build_Args $Assertions1 $Tight"}); 
     push (@$experiments, { "ENGINE" => "pdr"  , "BUILD_ARGS" => " $Build_Args $Assertions2 $Tight"}); 
     push (@$experiments, { "ENGINE" => "pdr"  , "BUILD_ARGS" => " $Build_Args $Assertions3 $Tight"}); 
}

Batch_Prove($experiments, $Exp_Name, $Timeout);
exit;

