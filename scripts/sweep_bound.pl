#!/usr/bin/env perl

use Carp;
use strict;
use Data::Dumper::Simple;
$Data::Dumper::Sortkeys = 1;
use Getopt::Long;

require "abc_utils.pl";

my $Exp_Name = "unnamed_experiment";
my $Timeout = 20;
my $Build_Args = ' ';
my @Assertions = ();

GetOptions (
	    "exp_name=s"    => \$Exp_Name, 
	    "timeout=i"     => \$Timeout, 
	    "build_args=s"  => \$Build_Args, 
	    "assertions=s"  => \@Assertions, 
	   ) or print "ERROR: bad input options"; 

print "\n";
print "="x70,"\n";; 
print "| ",$0,"\n";
print "| ","exp_name:             $Exp_Name\n";
print "| ","timeout:              $Timeout\n";
print "| ","build_args:           $Build_Args\n";
print "| ","assertions $_ =       $Assertions[$_] \n" foreach (0..$#Assertions);
print "="x70; 
print "\n";


my $Plot_Name = "$Exp_Name";


#############################################
# set the range based on cex and assertions #
#############################################
my $B_Min = 22;  
my $B_Max = 30;

my $out = `./dump.out $Build_Args --dump dump.v`;
my $tproved = parse_proved_bound($out);
$B_Max = $tproved + 5; 

my $tcex = find_t_cex({  "EXP_NAME"     => $Exp_Name.".tcex",
			 "T_MIN"        => 3,
			 "T_MAX"        => 100,
			 "TIMEOUT"      => $Timeout,
			 "MAX_FRAMES"   => 200,
			 "BUILD_ARGS"   => " $Build_Args --enable_tag phig_x --enable_tag psi "  });
if ($tcex !=0) {
     $B_Min = $tcex+1;		#smallest possible valid bound
} else {
     exit(1);
}
print "| ","b_min:                $B_Min\n";
print "| ","b_max:                $B_Max\n";



my $fmax = 1; 
open DAT, "> $Plot_Name.dat" or die "Can't open file : $!";

for my $b ($B_Min .. $B_Max) {

     printf DAT ("%10s, ",$b);

     for my $i (0..$#Assertions) {

	  system('./dump.out '.$Build_Args.' '.$Assertions[$i].'  --t_bound '.$b.' --dump dump.v');
	  my $r = abc_kind({   "INPUT_FILE" => "dump.v",
			       "MAX_FRAMES" => 500,
			       "TIMEOUT"    => $Timeout ,  
			       "EXP_NAME"   => $Exp_Name.".boundEQ".$b.".case$i"   });
	  print Dumper($r);

	  if ($r->{VALID} != 1) { 
	       $r->{FRAMES} = " ";
	       $r->{RUN_TIME} = " ";
	  } else {
	       $fmax = ($fmax > $r->{FRAMES}) ? $fmax : $r->{FRAMES};
	  }

	  printf DAT ("%10s, %10s, ", $r->{FRAMES}, $r->{RUN_TIME});

     }
     print DAT ("\n");
}
close DAT;

#vertical lines for tightest possible bound and bound proved by lemmas
open DAT, "> $Plot_Name.lines.dat" or die "Can't open file : $!";
printf DAT ("%10s, %10s, %10s, %10s \n",$B_Min , 0,      $tproved, 0);
printf DAT ("%10s, %10s, %10s, %10s \n",$B_Min , $fmax , $tproved, $fmax);
close DAT;

open GP, "> $Plot_Name.gp" or die "Can't open file : $!";
print GP "set terminal postscript eps enhanced color 20; \n";
print GP "set size 1.0,1.0; \n";
print GP "set output '$Plot_Name.eps';\n";
print GP "set multiplot;\n";

print GP "set size 1.0,0.5;\n";
print GP "set origin 0,0.5;\n";
print GP "set key Left rmargin;\n";
print GP "set key spacing 1.7;\n";
print GP "set grid xtics ytics;\n";
print GP "set format y '%5g';\n";
print GP "set xlabel ' ';\n";
print GP "set ylabel 'Runtime [s]';\n";
print GP "set xrange [".($B_Min-1).":".($B_Max+1)."];\n";
print GP "set yrange [0:];\n";
print GP "# assertions $_ = $Assertions[$_] = ".propdef_to_gp_string($Assertions[$_])."\n" foreach (0..$#Assertions);

print GP "plot \\\n";
for my $i (0..$#Assertions) {
     my $lc = $i+1;
     my $col = 2*$i + 3;
     print GP "     '$Plot_Name.dat' u 1:$col w lp lt 2 lw 8 ps 1.75 lc $lc pt $lc t '".propdef_to_gp_string($Assertions[$i])."'  ";
     print GP ($i != $#Assertions)? ",\\\n " : ";\n";
}

print GP "set size 1.0,0.5;\n";
print GP "set origin 0,0;\n";
print GP "set xlabel 'T: Global Bound Checked by {/Symbol F}_T^G';\n";
print GP "set ylabel 'Induction Depth';\n";
print GP "set xrange [".($B_Min-1).":".($B_Max+1)."];\n";
print GP "set yrange [0:];\n";
print GP "plot \\\n";
print GP "     '$Plot_Name.lines.dat' u 1:2 w l lw 9 lc rgb 'black'     lt 1 notitle ,  \\\n  "; #tight bound
print GP "     '$Plot_Name.lines.dat' u 3:4 w l lw 9 lc rgb 'dark-gray' lt 1 notitle,  \\\n "; #bound implied by lemmas
for my $i (0..$#Assertions) {
     my $lc = $i+1;
     my $col = 2*$i + 2;
     print GP "     '$Plot_Name.dat' u 1:$col w lp lt 2 lw 8 ps 1.75 lc $lc pt $lc t '".propdef_to_gp_string($Assertions[$i])."' ";
     print GP ($i != $#Assertions)? ",\\\n " : ";\n";
}

close GP;

system ("gnuplot $Plot_Name.gp");
system ("epstopdf $Plot_Name.eps");

exit;
