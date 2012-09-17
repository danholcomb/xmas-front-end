#!/opt/local/bin/perl

use Carp;
use strict;
use Data::Dumper::Simple;
$Data::Dumper::Sortkeys = 1;

require "/Users/dan/xmas_front_end/xmas-front-end/scripts/abc_utils.pl";

use Getopt::Long;

my $Network = "ex_queue";
my $Timeout = 20;
my $Log_Dir = "log";
GetOptions (
	    "network=s"    => \$Network, 
	    "log_dir=s"    => \$Log_Dir, 
	    "timeout=i"    => \$Timeout, 
	   ) or print "ERROR: bad input options"; 

print "\n";
print "="x70,"\n";; 
print "| ",$0,"\n";
print "| ","log_dir:              $Log_Dir\n";
print "| ","network:              $Network\n";
print "| ","timeout:              $Timeout\n";
print "="x70; 
print "\n";


my $Timestamp = `date`;
my $Experiments = ();

# each experiment is one row of the table
push (@$Experiments, { "ENGINE" => "pdr" ,        
		       "PROP_DEF" => " --enable_phig",
		       "PROP_STRING" => '$\Phi^G$' });

push (@$Experiments, { "ENGINE" => "pdr" ,        
		       "PROP_DEF" => " --enable_phig --enable_psi",
		       "PROP_STRING" => '$\Psi \wedge \Phi^G$' });

push (@$Experiments, { "ENGINE" => "pdr" ,        
		       "PROP_DEF" => " --enable_lemmas --enable_phig --enable_psi",
		       "PROP_STRING" => '$\Phi^L \wedge \Psi \wedge \Phi^G$' });

push (@$Experiments, { "ENGINE" => "pdr" ,        
		       "PROP_DEF" => " --enable_lemmas --enable_phig --enable_psi --enable_bound_channel --enable_response_bound_channel",
		       "PROP_STRING" => '$\Phi^L \wedge \Psi \wedge \Phi^G$ \wedge Channel' });


push (@$Experiments, { "ENGINE" => "kind" ,        
		       "PROP_DEF" => " --enable_phig",
		       "PROP_STRING" => '$\Phi^G$' });

push (@$Experiments, { "ENGINE" => "kind" ,        
		       "PROP_DEF" => " --enable_psi --enable_phig",
		       "PROP_STRING" => '$\Psi \wedge \Phi^G$' });

push (@$Experiments, { "ENGINE" => "kind" ,        
		       "PROP_DEF" => " --enable_lemmas --enable_phig --enable_psi",
		       "PROP_STRING" => '$\Phi^L \wedge \Psi \wedge \Phi^G$' });

push (@$Experiments, { "ENGINE" => "kind" ,        
		       "PROP_DEF" => " --enable_lemmas --enable_phig --enable_psi --enable_bound_channel --enable_response_bound_channel",
		       "PROP_STRING" => '$\Phi^L \wedge \Psi \wedge \Phi^G$ \wedge Channel' });


print Dumper($Experiments);


# generate result for each experiment
my $R = ();
foreach my $i (0..$#{$Experiments}) {

     my $prop_def     = $Experiments->[$i]->{PROP_DEF};
     my $prop_string  = $Experiments->[$i]->{PROP_STRING};
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
     $r->{PROP_STRING} = "$prop_string";
     $r->{PROVED} = ($r->{VALID} == 1) ? "Y" : "-";

     my $foo = $Network;
     my $log_file_name = 'compare_engines__'.$foo.'_'.$i.'' ;
     $r->{LOG_FILE_NAME} = $log_file_name;

     push (@$R,$r);
     #print print_table($R);
     
     writeFile("current_results.txt",print_table($R)); # just to check on progress
     appendFile("current_results.txt",Dumper($R));

     system('mkdir '.$log_file_name.'');
     system('cp dump.abc dump.aig dump.v current_results.txt '.$log_file_name.'/');
     system('tar -czf log/'.$log_file_name.'.tar.gz '.$log_file_name.'/');
     system('rm -r '.$log_file_name.'');
}

appendFile("exp_results.txt",print_table($R));
print "exiting normally\n";
print Dumper($R);
exit;




sub print_table {
     my $r = shift;
     print Dumper($r);

     my $t = "% \n";
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
			 $r->[$i]->{PROP_STRING},
			);
     }
     return $t;

}

