#!/usr/bin/env perl

use Carp;
use strict;
use Data::Dumper::Simple;
$Data::Dumper::Sortkeys = 1;
use Getopt::Long;

require "abc_utils.pl";

my $Timeout = 20;
my $Build_Args = ' ';
my $Exp_Name = 'unnamed_experiment';
my $Sweep_Param = 'numStages';
my $P_Min = 1;
my $P_Max = 7;
my $Exponent = 2; #fill range exponentionally with this exponent
my $Log = ''; #logarithmic steps to fill range
my $Bmc = ''; #find tight bound using bmc
my $Terminate_On_Fail = ''; #quit sweeping if no engine can solve at current param val
my @Assertions = ();
my @Engine = ();

GetOptions (
	    "build_args=s"       => \$Build_Args, 
	    "exp_name=s"         => \$Exp_Name, 
	    "timeout=i"          => \$Timeout, 
	    "sweep_param=s"      => \$Sweep_Param, 
	    "p_min=i"            => \$P_Min, 
	    "p_max=i"            => \$P_Max, 
	    "exponent=f"         => \$Exponent, 
	    "log"                => \$Log, 
	    "bmc"                => \$Bmc, 
	    "terminate_on_fail"  => \$Terminate_On_Fail, 
	    "assertions=s"  => \@Assertions, 
	    "engine=s"      => \@Engine, 
	   ) or print "ERROR: bad input options"; 

($#Assertions == $#Engine) or croak;
print "\n";
print "="x70,"\n";; 
print "| ",$0,"\n";
print "| ","build_args:           $Build_Args\n";
print "| ","exp_name:             $Exp_Name\n";
print "| ","timeout:              $Timeout\n";
print "| ","sweep_param:          $Sweep_Param\n";
print "| ","p_min:                $P_Min\n";
print "| ","p_max:                $P_Max\n";
print "| ","assertions $_ =       $Engine[$_] : $Assertions[$_] \n" foreach (0..$#Assertions);
print "="x70; 
print "\n";


my $Plot_Name = "$Exp_Name";

my @P_Vals = ($P_Min .. $P_Max);
if ($Log) {
     @P_Vals = ($P_Min); 
     while ($P_Vals[-1]*$Exponent < $P_Max) { push(@P_Vals,int($P_Vals[-1]*$Exponent)); }
} 
print Dumper(@P_Vals);

open DAT, "> $Plot_Name.dat" or die "Can't open file : $!";
my $tmin = 3;
for my $p (@P_Vals) {
     print "starting iteration $p \n";


     my $out = `./dump.out $Build_Args --$Sweep_Param $p --dump dump.v`;
     
     if ($Bmc) {
	  my $tproved = parse_proved_bound($out);
	  my $tcex = find_t_cex({  "EXP_NAME"     => $Exp_Name.".".$Sweep_Param."EQ".$p.".tcex",
				   "T_MIN"        => 3,
				   "T_GUESS"      => $tmin,
				   "T_MAX"        => 100,
				   "TIMEOUT"      => $Timeout,
				   "MAX_FRAMES"   => 200,
				   "BUILD_ARGS"   => " $Build_Args --".$Sweep_Param." $p --enable_tag phig_x --enable_tag psi "
				});
	  
	  my $tfeas; 
	  if ($tcex == 0) {
	       $tfeas = " ";
	  } else {
	       $tfeas = $tcex + 1; #makes reasonable assumption that tight bound is cex+1 for plot clarity
	       $tmin = $tcex; #next iteration must have longer bound due to larger depths
	  }
	  printf DAT ("%10s, %10s, %10s, ",$p, $tfeas, $tproved);
     } else {
	  printf DAT ("%10s, %10s, %10s, ",$p, 0, 0);
     }


     my $continue_sweep = 0; #stop sweep if everything fails
     foreach my $i (0..$#Assertions) {
	  system('./dump.out '.$Build_Args.' --'.$Sweep_Param.' '.$p.' '.$Assertions[$i].' --dump dump.v');
	  my $r = abc_x({ "ENGINE"     => $Engine[$i] ,
			  "INPUT_FILE" => "dump.v",
			  "MAX_FRAMES" => 1000,
			  "TIMEOUT"    => $Timeout,   	
			  "EXP_NAME"   => $Exp_Name.".".$Sweep_Param."EQ".$p.".case".$i   });
	  print Dumper($r);
	  
	  if ($r->{VALID} != 1) { 
	       $r->{FRAMES} = " ";
	       $r->{RUN_TIME} = " ";
	  } else {
	       $continue_sweep = 1;
	  }
	  

	  printf DAT ("%10s, %10s, " ,$r->{RUN_TIME}, $r->{FRAMES} );

     }
     
     printf DAT ("\n");
     last if ($Terminate_On_Fail and not $continue_sweep);
     
 }
close DAT;

open GP, "> $Plot_Name.gp" or die "Can't open file : $!";
print GP "set terminal postscript eps enhanced color 20; \n";
print GP "set size 1.0,1.0; \n";
print GP "set output '$Plot_Name.eps';\n";
print GP "set multiplot;\n";

print GP "set size 1.0,0.5;\n";
print GP "set origin 0,0.5;\n";
#print GP "set key Left left;\n";
print GP "set key Left rmargin;\n";
print GP "set key spacing 1.2;\n";
# print GP "set key spacing 1.7;\n";
print GP "set grid xtics ytics;\n";
print GP "set format y '%5g';\n";
print GP "set xlabel ' ';\n";
print GP "set ylabel 'Runtime [s]';\n";
print GP "set xrange [$P_Min:$P_Max];\n";
print GP "set yrange [0.001:]; \n";
# print GP "set yrange [0.01:]; \n";
#print GP "set logscale y;\n";
print GP "set logscale x;\n" if ($Log);



my @titles = map {$Engine[$_]."; ".propdef_to_gp_string($Assertions[$_])} (0..$#Assertions);
print GP "#  $_ = $Assertions[$_] = $titles[$_] \n" foreach (0..$#Assertions);

print GP "plot \\\n";
for my $i (0..$#Assertions) {
     my $lc = $i+1;
     my $col = 2*$i + 4;
     print GP "     '$Plot_Name.dat' u 1:$col w lp lc $lc pt $lc lt $lc lw 7 ps 2 t '$titles[$i]'  ";
     print GP ($i != $#Assertions)? ",\\\n " : ";\n";
}

print GP "set size 1.0,0.5;\n";
print GP "set origin 0,0;\n";
print GP "set xlabel '".remove_underscore($Sweep_Param)."';\n";
print GP "set ylabel 'Induction Depth';\n";
print GP "set xrange [$P_Min:$P_Max];\n";
print GP "set yrange [0:];\n";
print GP "set key invert;\n";
print GP "unset logscale;\n";
print GP "set logscale x;\n" if ($Log);
print GP "plot \\\n";
print GP "     '$Plot_Name.dat' u 1:3 w lp lt 2 lw 8 ps 1.75 lc rgb 'dark-gray' pt 8  t 'T_L',      \\\n" if ($Bmc);
print GP "     '$Plot_Name.dat' u 1:2 w lp lt 2 lw 8 ps 1.75 lc rgb 'black'     pt 10 t 'T_{FEAS}', \\\n" if ($Bmc);

#reverse order so that invert will do right thing
for my $j (0..$#Assertions) {
     my $i = $#Assertions-$j;
     my $lc = $i+1;
     my $col = 2*$i + 5;
     print GP "     '$Plot_Name.dat' u 1:$col w lp lt 2 lw 8 ps 1.75 lc $lc pt $lc t '$titles[$i]'   ";
     print GP ($j != $#Assertions)? ",\\\n " : ";\n";
}

# print GP join(' ,\\\n' ,@plotlines);



close GP;

system ("gnuplot $Plot_Name.gp");
system ("epstopdf $Plot_Name.eps");

exit;
