#!/opt/local/bin/perl

use Carp;
use strict;
use Data::Dumper::Simple;
$Data::Dumper::Sortkeys = 1;
use Getopt::Long;

require "/Users/dan/xmas_front_end/xmas-front-end/scripts/abc_utils.pl";


my $Network = "ex_queue";
my $Timeout = 20;
my $Log_Dir = "log";
my $T_Max = -1;
GetOptions (
	    "network=s"    => \$Network, 
	    "log_dir=s"    => \$Log_Dir, 
	    "timeout=i"    => \$Timeout, 
	    "t_max=i"      => \$T_Max, 
	   ) or print "ERROR: bad input options"; 

print "\n";
print "="x70,"\n";; 
print "| ",$0,"\n";
print "| ","log_dir:              $Log_Dir\n";
print "| ","network:              $Network\n";
print "| ","timeout:              $Timeout\n";
print "| ","t_max:                $T_Max\n";
print "="x70; 
print "\n";


my $Timestamp = `date`;
my $Experiments = ();

if (($Network eq "credit_loop")) {
     push (@$Experiments, { "ENGINE" => "pdr" ,        
			    "PROP_DEF" => " --enable_phig --enable_persistance --enable_psi --t_max $T_Max"});

     push (@$Experiments, { "ENGINE" => "pdr" ,        
			    "PROP_DEF" => " --enable_phig --enable_persistance --enable_psi --enable_num_inv --t_max $T_Max" });

     push (@$Experiments, { "ENGINE" => "kind" ,        
			    "PROP_DEF" => " --enable_phig --enable_persistance --enable_psi --t_max $T_Max" });

     push (@$Experiments, { "ENGINE" => "kind" ,        
			    "PROP_DEF" => " --enable_phig --enable_persistance --enable_psi --enable_num_inv --t_max $T_Max" });

} else {
     # these settings assume that propagation succeeds, and will cause problems otherwise
     # each experiment is one row of the table
     push (@$Experiments, { "ENGINE" => "pdr" ,        
			    "PROP_DEF" => " --enable_phig" });

     push (@$Experiments, { "ENGINE" => "pdr" ,        
			    "PROP_DEF" => " --enable_phig --enable_psi" });

     push (@$Experiments, { "ENGINE" => "pdr" ,        
			    "PROP_DEF" => " --enable_lemmas --enable_phig --enable_psi" });

     push (@$Experiments, { "ENGINE" => "pdr" ,        
			    "PROP_DEF" => " --enable_lemmas --enable_phig --enable_psi --enable_bound_channel --enable_response_bound_channel" });

     push (@$Experiments, { "ENGINE" => "kind" ,        
			    "PROP_DEF" => " --enable_psi --enable_phig" });

     push (@$Experiments, { "ENGINE" => "kind" ,        
			    "PROP_DEF" => " --enable_lemmas --enable_phig --enable_psi" });

     push (@$Experiments, { "ENGINE" => "kind" ,        
			    "PROP_DEF" => " --enable_lemmas --enable_phig --enable_psi --enable_bound_channel --enable_response_bound_channel" });

}
print Dumper($Experiments);


# generate result for each experiment
my $R = ();
foreach my $i (0..$#{$Experiments}) {
     my $prop_def     = $Experiments->[$i]->{PROP_DEF};
     my $engine       = $Experiments->[$i]->{ENGINE};

     system('./dump.out --network '.$Network.' '.$prop_def.' --dump dump.v');
     system('v2aig.sh dump.v');
     system('ulimit -St '.$Timeout.'; abc-'.$engine.'.sh dump.aig');     
     my $out = readFileScalar('dump.abc');

     my $r = parse_abc($engine,$out);
     if ($r->{TIMEOUT}) {
	  $r->{RUN_TIME} = $Timeout;
     }
     $r->{ENGINE} = "$engine";
     $r->{PROP_DEF} = "$prop_def";
     $r->{PROVED} = ($r->{VALID} == 1) ? "Y" : "-";

     my $log_file_name = 'compare_engines__'.$Network.'_'.$i.'' ;
     $r->{LOG_FILE_NAME} = $log_file_name;
     push (@$R,$r);

     writeFile("current_results.txt",print_table($R)); # just to check on progress
     appendFile("current_results.txt",Dumper($R));

     system('mkdir -p '.$log_file_name.'');
     system('cp abc.rc dump.abc dump.aig dump.v current_results.txt '.$log_file_name.'/');
     system('tar -czf '.$Log_Dir.'/'.$log_file_name.'.tar.gz '.$log_file_name.'/');
     system('rm -r '.$log_file_name.'');
}

appendFile("exp_results.txt",print_table($R));
print "exiting normally\n";
print Dumper($R);
exit;


sub propdef_to_propstring {
     my $d = shift;
     my @symbols = ();
     push (@symbols, '\Phi^L')  if ($d =~ m/--enable_lemmas/);
     push (@symbols, '\Phi^G')  if ($d =~ m/--enable_phig/);
     push (@symbols, '\Psi')    if ($d =~ m/--enable_psi/);
     push (@symbols, 'Num')    if ($d =~ m/--enable_num_inv/);
     push (@symbols, 'Channel') if ($d =~ m/--enable_bound_channel/ and $d =~ m/enable_response_bound_channel/);
     my $s = "\$".join(' \wedge ',@symbols)."\$";
     return $s;
}


sub print_table {
     my $r = shift;
     print Dumper($r);

     my $t = "\n% \n";
     $t .= "% ".$0."\n";
     $t .= "% ".$Network."\n";
     $t .= "% ".$Timestamp;
     $t .= "% ".Dumper($Timeout);
     $t .= sprintf ("%12s & %6s & %6s & %6s & %42s \\\\ \\hline \\hline\n",
		    "Runtime (s)", "Frames", "Proved", "Engine","Property");

     foreach my $i (0..$#$r) {
	  $t .= sprintf ("%12.2f & %6s & %6s & %6s & %50s \\\\ \\hline \n",
			 $r->[$i]->{RUN_TIME},
			 $r->[$i]->{FRAMES},
			 $r->[$i]->{PROVED},
			 $r->[$i]->{ENGINE},
			 propdef_to_propstring($r->[$i]->{PROP_DEF}),
			);
     }
     return $t;
}

