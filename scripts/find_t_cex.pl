#!/opt/local/bin/perl

use Carp;
use strict;
use Data::Dumper;
$Data::Dumper::Sortkeys = 1;
use Getopt::Long;

require "/Users/dan/xmas_front_end/xmas-front-end/scripts/abc_utils.pl";

my $Network = "credit_loop";
my $T_Max = 30;
my $T_Min  = 0;
my $Bmc_Timeout = 20;
my $Bmc_Frames = 100;
my $Log_Dir = "log";
GetOptions (
	    "network=s"     => \$Network, 
	    "log_dir=s"     => \$Log_Dir, 
	    "t_max=i"       => \$T_Max, 
	    "t_min=i"       => \$T_Min, 
	    "bmc_timeout=i" => \$Bmc_Timeout, 
	    "bmc_frames=i"  => \$Bmc_Frames, 
	   ) or print "ERROR: bad input options"; 

print "\n";
print "="x70,"\n";; 
print "| ",$0,"\n";
print "| ","log_dir:              $Log_Dir\n";
print "| ","network:              $Network\n";
print "| ","t_max:                $T_Max\n";
print "| ","t_min:                $T_Min \n";
print "| ","bmc_timeout:          $Bmc_Timeout\n";
print "| ","bmc_frames:           $Bmc_Frames\n";
print "="x70; 
print "\n";



my $R = ();
my $Timestamp = `date`;
chomp ($Timestamp);

#my $Search_Type = "BINARY";
my $Search_Type = "LINEAR";


#my $t_test = 1;
my $t_max = $T_Max;
my $t_min = $T_Min;
my $t_cex = 0;
my $nn = 0;

system('rm -rf '.$Log_Dir.'');
system('mkdir '.$Log_Dir.'');

system('make dump.out');
my $t_test = $t_min; 

 
while (($t_min <= $t_test) and ($t_test <= $t_max)) {
     croak if ($nn++ == 20); #just in case ?
     
     system('./dump.out --network '.$Network.' --t_max '.$t_test.' --disable_lemmas --disable_psi --disable_persistance --disable_bound_channel --disable_response_bound_channel --dump dump.v');
     
     system('ulimit -St '.$Bmc_Timeout.'; abc-bmc.sh dump.v '.$Bmc_Frames.'');

     my $out = readFileScalar('dump.abc');
     my $r = parse_bmc($out);
     if ($r->{TIMEOUT}) {
	  $r->{RUN_TIME} = $Bmc_Timeout;
     }
	  
     if ($r->{INVALID}==1) {
	  $t_cex = $t_test;
	  $t_min = $t_test + 1;
     } elsif (($r->{INVALID}==0) or ($r->{TIMEOUT}==1)) {
	  if ($Search_Type eq "BINARY") {
	       $t_max = $t_test - 1;
	  } elsif ($Search_Type eq "LINEAR") {
	       $t_max = 0; #end search whenever get the first case without a cex.	       
	  } else {croak;}
     } else {
	  croak "ERROR: dont understand results\n".Dumper($r)."\n" ;
     }
	  
     my $foo = $Network;
     $foo =~ s/\.v$//;
     my $log_file_name = 'find_t_cex__'.$foo.'_'.$t_test.'' ;
     $r->{LOG_FILE_NAME} = $log_file_name;
     $r->{T_TEST} = $t_test;
     $r->{T_CEX} = $t_cex;
     push (@$R,$r);

     print Dumper($R);
     print print_table($R);

     writeFile("current_results.txt",print_table($R));
     #appendFile("current_results.txt",Dumper($R));

     system('mkdir '.$log_file_name.'');
     system('cp dump.abc dump.v dump.aig current_results.txt '.$log_file_name.'/');
     system('tar -czf '.$Log_Dir.'/'.$log_file_name.'.tar.gz '.$log_file_name.'/');
     system('rm -r '.$log_file_name.'');
     printf("test:%5d min:%5d max:%5d cex:%5d\n",$t_test,$t_min,$t_max,$t_cex);

     # set for next time
     if ($Search_Type eq "BINARY") {
	  $t_test = int (($t_max + $t_min)/2);
     } elsif ($Search_Type eq "LINEAR") {
	  $t_test = $t_min; 
     }
	  
}

appendFile("exp_results.txt",print_table($R));

#print Dumper($R);
print "exiting normally\n";
exit;



sub print_table {
     my $r = shift;

     my $foo = $Network;
     $foo =~ s/_/\\_/g;

     my $t = "\n";
     $t .= "%\n";
     $t .= "%\n";
     $t .= "% ".$0."\n";
     $t .= "% timestamp   ".$Timestamp."\n";
     $t .= "% network     ".$Network."\n";
     $t .= "% bmc_timeout ".$Bmc_Timeout."\n";
     $t .= "% bmc_frames  ".$Bmc_Frames."\n";
     $t .= "\\hline \n";
     $t .= '\multirow{'.(2+$#{$r}).'}{*}{{ '.$foo.'}}'."\n"; 
     $t .= sprintf ("& %10s & %10s & %10s & %10s \\\\ \\cline{2-5}\n",
		    "T", 
		    "Found CEX", 
		    "Runtime(s)", 
		    "Frames");


     foreach my $i (sort {$a->{T_TEST} <=> $b->{T_TEST}} @{$r}) {
	  $t .= sprintf ("& %10i & %10i & %10.2f & %10i  \\\\  \n",			 
			 $i->{T_TEST},
			 $i->{INVALID},
			 $i->{RUN_TIME},
			 $i->{FRAMES},
			);
     }
     return $t;

}

