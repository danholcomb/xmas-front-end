

use Carp;
use strict;
use Data::Dumper::Simple;
$Data::Dumper::Sortkeys = 1;


sub convert_to_aig {
     my $infile = shift;
     if ($infile =~ /.*\.v/) {
	  print "\nconverting v to aig\n";
	  (system("v2aig.sh $infile") == 0) or croak;
	  (-e 'dump.aig') or croak "dump.aig not created";
	  return "dump.aig";
     } else {
	  croak unless ($infile =~ /.*\.aig/);
	  return $infile;
     }
}


sub parse_proved_bound {
     my $dump = shift;
     if ($dump =~ /max age bound at any location is (\d+)/) {
	  return $1;
     } else {
	  croak;
     }
}




#
# 
#
sub  parse_kind {
     my $out = shift;
     my $k = 0;
     my $r = {
	      "FRAMES"   => -1,
	      "VALID"    => -1,
	      "RUN_TIME" => -1,
	      "TIMEOUT"  => 0
	     };

     if ($out =~ /Completed (\d+) interations[\w\s]*.[\W]*Networks are equivalent.\s+Time =\s+([\d\.]+) sec/) {
	  $r->{VALID}    = 1;
	  $r->{FRAMES}   = $1;
	  $r->{RUN_TIME} = $2;

	  # the frames from regex doesn't seem to be induction depth,
	  # instead use the largest frame explored before concluding
	  my @lines = split(/[\n\r]/,$out);
	  my $max_frame = -1;
	  foreach my $line (grep {/^\s*\d+\s*:\s*[01x]+$/} @lines) {
	       my $frame = $1 if ($line =~ /\s*(\d+)\s*:.*/);
	       $max_frame = ($max_frame > $frame) ? $max_frame : $frame;
	  }
	  $r->{FRAMES} = $max_frame;


     } elsif ($out =~ /Completed (\d+) interations[\w\s]*.[\W]*Networks are UNDECIDED.\s+Time =\s+([\d\.]+) sec/) {
	  $r->{VALID}    = 0;
	  $r->{FRAMES}   = $1;
	  $r->{RUN_TIME} = $2;
     } else {
	  writeFile("abc.out",Dumper($out,$r));
	  carp "\n\nPossible timeout - cannot understand results";

	  # find the largest frame explored 
	  my @lines = split(/[\n\r]/,$out);
	  my $max_frame = -1;
	  foreach my $line (grep {/^\s*\d+\s*:\s*[01x]+$/} @lines) {
	       my $frame = $1 if ($line =~ /\s*(\d+)\s*:.*/);
	       $max_frame = ($max_frame > $frame) ? $max_frame : $frame;
	  }
	  $r->{FRAMES}  = $max_frame;
	  $r->{TIMEOUT} = 1;
     }
     
     writeFile("abc.out",Dumper($out,$r));

     # print "\n\n".Dumper($out,$r);
     return $r;
}



sub  parse_assertions {
     my $fname = shift;
     my @a = ();
     my $out = readFileScalar($fname);

     my @lines = split(/[\n\r]/,$out);
     #print Dumper(@lines);
     foreach my $i (0..$#lines) { 
	  if ($lines[$i] =~ /^\s*xmas_assert\s+.*,\s*(\w+)\s*\);\s*$/) { 
	       push(@a, $1);
	  }
     }
     #print Dumper(@a);
     return @a;
}





#
# parse the dump from a run of PDR
#
sub  parse_pdr {
     my $out = shift;

     my $r = {
	      "FRAMES" => -1,
	      "VALID" => -1,
	      "RUN_TIME" => -1,
	      "TIMEOUT" => 0
	     };

     if ($out =~ /Reached limit on the number of timeframes \((\d+)\).[\w\W]+Property UNDECIDED.\s+Time =\s+([\d\.]+) sec/) {
	  $r->{VALID}    = 0;
	  $r->{FRAMES}   = $1;
	  $r->{RUN_TIME} = $2;
     } elsif ($out =~ /Property DISPROVED in frame (\d+) [\w\W]+.\s+Time =\s+([\d\.]+) sec/) {
	  $r->{VALID}    = 0;
	  $r->{FRAMES}   = $1;
	  $r->{RUN_TIME} = $2;
     } elsif ($out =~ /Invariant F\[(\d+)\] :[\w\W]+Property proved.\s+Time =\s+([\d\.]+) sec/) {
	  $r->{VALID}    = 1;
	  $r->{FRAMES}   = $1;
	  $r->{RUN_TIME} = $2;
     } else {
	  writeFile("abc.out",Dumper($out,$r));
	  carp "\n\nPossible timeout - cannot understand results";
	  
	  # find the largest frame explored
	  my @lines = split(/[\n\r]/,$out);
	  my $max_frame = 0;
	  foreach my $line (grep {/\s*\d+\s*:[\s\d\.]+[\d\.]+ sec/} @lines) {
	       my $frame = $1 if ($line =~ /\s*(\d+)\s*:.*/);
	       $max_frame = ($max_frame > $frame) ? $max_frame : $frame;
	  }
	  $r->{FRAMES}  = $max_frame;
	  $r->{TIMEOUT} = 1;
     }

     writeFile("abc.out",Dumper($out,$r));
     return $r;
}









sub  abc_x {
     my ($args) = @_;
     
     if      ($args->{"ENGINE"} eq "bmc") {
	  return abc_bmc($args); 
     } elsif ($args->{"ENGINE"} eq "kind") {
	  return abc_kind($args); 
     } elsif ($args->{"ENGINE"} eq "pdr") {
	  return abc_pdr($args); 
     } else { 
	  croak;
     }

}







#
# run a single instance of k-induction with ABC, returns the results
#
sub  abc_kind {
     my ($args) = @_;

     my $input_file    = $args->{"INPUT_FILE"};
     my $k             = $args->{"MAX_FRAMES"};
     my $timeout       = $args->{"TIMEOUT"};
     my $exp_name      = $args->{"EXP_NAME"};

     print "\nabc_kind() ","="x60;
     print "\n".Dumper($args);

     my $aig_file = convert_to_aig($input_file);
     
#      writeFile("abc_kind.script","version; read_aiger $aig_file; print_stats; orpos; \nind -uvw -F $k -C 1000000000;\n");
# -a means always add uniqueness, -u means uniqueness constraints on-demand

     writeFile("abc_kind.script","version; read_aiger $aig_file; print_stats; orpos; \nind -avw -F $k -C 1000000000;\n");
     my $out = `ulimit -St $timeout; abc -F abc_kind.script`; 
          
     my $r_kind = parse_kind($out);
     if ($r_kind->{TIMEOUT} == 1) {
	  $r_kind->{RUN_TIME} = $timeout;
     }


     if ($r_kind->{VALID} == 1) {
	  print "induction holds, now running bmc to confirm...\n";
 	  my $r_bmc = abc_bmc({  "INPUT_FILE" => $aig_file, 
				 "MAX_FRAMES" => $r_kind->{FRAMES},
				 "TIMEOUT"    => $timeout,
				 "EXP_NAME"   => $exp_name.".kind_bmc",
			      });
	  if ($r_bmc->{VALID} == 0) {
	       $r_kind->{VALID} = 0;
	       print "bmc fails, induction invalid\n";
	       print Dumper($args);
	       print Dumper($r_kind);
	       print Dumper($r_bmc);
	       croak; 
	  } else {
	       print "bmc holds, induction valid\n";
	       $r_kind->{RUN_TIME} = $r_kind->{RUN_TIME} + $r_bmc->{RUN_TIME};
	  }
     }

     
     writeFile("abc.out",Dumper($out,$r_kind));
     system('mkdir log ');
     system('tar -czf log/'.$exp_name.'.tar.gz abc_kind.script dump.abc abc.out '.$aig_file.' '.$input_file.' ');
     system("rm abc_kind.script");
     print Dumper($r_kind);
     print "","="x60,"\n";

     appendFile("abc_runs.log","\n\nKIND: ".`date`.Dumper($exp_name,$args,$r_kind)."\n\n");
     return $r_kind;
}



#
# run a single instance of bmc with ABC, returns the results
#
sub  abc_bmc {
     my ($args) = @_;

     my $input_file    = $args->{"INPUT_FILE"};
     my $f             = $args->{"MAX_FRAMES"};
     my $timeout       = $args->{"TIMEOUT"};
     my $exp_name      = $args->{"EXP_NAME"};
     
     print "\nabc_bmc() ","="x60;
     print "\n".Dumper($args);
     my $aig_file = convert_to_aig($input_file);

     writeFile("abc.script","version; read_aiger $aig_file; print_io -f; bmc3 -v -F $f -C 1000000; write_counter -n foo.cex;\n");

     my $out = `ulimit -St $timeout; abc -F abc.script | tee dump.abc`; 

     my $r = {
	      "MAX_FRAMES" => $f,
	      "FRAMES"     => -1,
	      "VALID"      => -1,
	      "INVALID"    => -1,
	      "RUN_TIME"   => -1,
	     };

     if ($out =~ /Output \d+ asserted in frame (\d+) .*. Time\s+=\s+([\d\.]+) sec/) {
	  $r->{VALID}    = 0;
	  $r->{INVALID}  = 1;
	  $r->{FRAMES}   = $1;
	  $r->{RUN_TIME} = $2;
     } elsif ($out =~ /No output asserted in (\d+) frames..*Time\s+=\s+([\d\.]+) sec/) {
	  $r->{VALID}    = 1;
	  $r->{INVALID}  = 0;
	  $r->{FRAMES}   = $1;
	  $r->{RUN_TIME} = $2;
     } else {
	  print Dumper($out);
	  carp "cannot understand results, possibly a timeout";

	  my @lines = split(/[\n\r]/,$out);
	  my $max_frame = 0;
	  foreach my $line (grep {/\s*\d+\s*\+\s*:.+\s+[\d\.]+ sec/} @lines) {
	       my $frame = $1 if ($line =~ /\s*(\d+)\s*\+\s*:.*/);
	       $max_frame = ($max_frame > $frame) ? $max_frame : $frame;
	  }
	  $r->{VALID}    = 1;
	  $r->{INVALID}  = 0;
	  $r->{FRAMES}   = $max_frame;
	  $r->{RUN_TIME} = $timeout;

     }

     writeFile("abc.out",Dumper($out,$r));

     system('mkdir log ');
     system('tar -czf log/'.$exp_name.'.tar.gz dump.abc abc.out abc.script '.$aig_file.' '.$input_file.' ');
     system("rm abc.script");

     print Dumper($r);
     print "","="x60,"\n";
     appendFile("abc_runs.log","\n\nBMC: ".`date`.Dumper($exp_name,$args,$r)."\n\n");
     return $r;
}







#
# run a single instance of bmc with ABC, returns the results
#
sub  abc_pdr {
     my ($args) = @_;

     my $input_file    = $args->{"INPUT_FILE"};
#     my $f             = $args->{"FRAMES"};
     my $f             = $args->{"MAX_FRAMES"};
     my $timeout       = $args->{"TIMEOUT"};
     my $exp_name      = $args->{"EXP_NAME"};

     print "\nstarting pdr : ".Dumper($args);
     my $aig_file = convert_to_aig($input_file);
     
     writeFile("abc.script","read_aiger $aig_file; \npdr -F $f -C 100000000 -v;\n");
     my $out = `ulimit -St $timeout; abc -F abc.script`; 
     my $r = parse_pdr($out);
     
     print "\npdr output \n".Dumper($r);

     writeFile("abc.out",Dumper($out,$r));
     system('mkdir log ');
     system('tar -czf log/'.$exp_name.'.tar.gz dump.abc abc.out abc.script '.$aig_file.' '.$input_file.' ');
     system("rm abc.script");

     appendFile("abc_runs.log","\n\nPDR: ".`date`.Dumper($exp_name,$args,$r)."\n\n");
     return $r;
}



# sub find_t_cex {
#      my ($args) = @_;
     
#      my $exp_name     = $args->{"EXP_NAME"};
#      my $t_min        = $args->{"T_MIN"};
#      my $t_max        = $args->{"T_MAX"};
#      my $bmc_timeout  = $args->{"TIMEOUT"};
#      my $bmc_frameout = $args->{"MAX_FRAMES"};
#      my $build_args   = $args->{"BUILD_ARGS"};

#      my $timestamp = `date`;
#      chomp ($timestamp);
#      $args->{TIMESTAMP} = $timestamp;

#      print "\n";
#      print "="x70,"\n";; 
#      print "| ",$0,"\n";
#      print "| ","exp_name:             $exp_name\n";
#      print "| ","t_min:                $t_min \n";
#      print "| ","t_max:                $t_max\n";
#      print "| ","bmc_timeout:          $bmc_timeout\n";
#      print "| ","bmc_frameout:         $bmc_frameout\n";
#      print "| ","build_args:           $build_args\n";
#      print "| ","timestamp:            $timestamp\n";
#      print "="x70; 
#      print "\n";

#      #my $Search_Type = "BINARY";
#      my $Search_Type = "LINEAR";

#      my $t_cex = 0;
# #     my $nn = 0;

#      my $t_test = $t_min; 
     
 
#      my $R = ();
#      my @t_tested = ();
#      while (($t_min <= $t_test) and ($t_test <= $t_max)) {
# 	  #croak if ($nn++ == 70); #just in case ?
# 	  croak if ($#t_tested > 70); #just in case ?
# 	  # use psi invariants because it speeds up BMC
# 	  # This is sound/convservative, because we only care about finding the unsat result
# 	  (system('./dump.out '.$build_args.' --t_bound '.$t_test.' --dump dump.v') == 0) or croak;

# 	  my $r = abc_bmc({  "INPUT_FILE" => "dump.v", 
# 			     "MAX_FRAMES" => $bmc_frameout,
# 			     "TIMEOUT"    => $bmc_timeout,
# 			     "EXP_NAME"   => $exp_name.".boundEQ".$t_test,
# 			    });

# 	  print Dumper($r);
	  
# 	  if ($r->{INVALID}==1) {
# 	       $t_cex = $t_test;
# 	       $t_min = $t_test + 1;
# 	  } elsif (($r->{INVALID}==0) or ($r->{TIMEOUT}==1)) {
# 	       if ($Search_Type eq "BINARY") {
# 		    $t_max = $t_test - 1;
# 	       } elsif ($Search_Type eq "LINEAR") {
# 		    if ($t_cex == 0) { 
# 			 ($t_min > 2) or croak;
# 			 $t_min = 2;  #never found any cex, so change range to find cex
# 		    } else {
# 			 $t_max = 0; #end search whenever get the first case without a cex.	       
# 		    } 
		    
# 	       } else {
# 		    croak;
# 	       }
# 	  } else {
# 	       croak "ERROR: dont understand results\n".Dumper($r)."\n" ;
# 	  }
	  
# 	  my $log_file_name = ''.$exp_name.'__TTEST'.$t_test.'' ;
# 	  $r->{LOG_FILE_NAME} = $log_file_name;
# 	  $r->{T_TEST} = $t_test;
# 	  $r->{T_CEX} = $t_cex;
# 	  push (@$R,$r);

# 	  print Dumper($R);
# 	  print sprint_bmc_table($R,$args);

# 	  writeFile($exp_name.".tex",sprint_bmc_table($R,$args));

# # 	  writeFile("current_results.txt",sprint_bmc_table($R,$args));
# 	  printf("test:%5d min:%5d max:%5d cex:%5d\n",$t_test,$t_min,$t_max,$t_cex);

# 	  # set for next time
# 	  if ($Search_Type eq "BINARY") {
# 	       $t_test = int (($t_max + $t_min)/2);
# 	  } elsif ($Search_Type eq "LINEAR") {
# 	       $t_test = $t_min; 
# 	  }
	  
#      }

#      appendFile("exp_results.txt",sprint_bmc_table($R,$args));
#      print "find_t_cex finished, tcex = $t_cex\n";
#      return $t_cex;

# }



sub find_t_cex {
     my ($args) = @_;
     
     my $exp_name     = $args->{"EXP_NAME"};
     my $t_min        = $args->{"T_MIN"};
     my $t_max        = $args->{"T_MAX"};
     my $bmc_timeout  = $args->{"TIMEOUT"};
     my $bmc_frameout = $args->{"MAX_FRAMES"};
     my $build_args   = $args->{"BUILD_ARGS"};
     my $t_test       = (defined $args->{"T_GUESS"}) ? $args->{"T_GUESS"} : $args->{"T_MIN"};
     my $param_name   = (defined $args->{"PARAM_NAME"}) ? $args->{"PARAM_NAME"} : 't_bound';

     my $timestamp = `date`;
     chomp ($timestamp);
     $args->{TIMESTAMP} = $timestamp;
     
     print "\n","="x70,"\n";; 
     print "| ",$0,"\n";
     printf ("| %-20s %-10s\n",$_,$args->{$_}) foreach (keys %$args);
     print "="x70,"\n";

     my $t_cex = 0;
     my $R = ();
     while ($t_min < $t_max) {
	  croak if ($#$R > 200); #just in case ?

	  #(system('./dump.out '.$build_args.' --t_bound '.$t_test.' --dump dump.v') == 0) or croak;
	  (system('./dump.out '.$build_args.' --'.$param_name.' '.$t_test.' --dump dump.v') == 0) or croak;

	  my $exp_name_bound = $exp_name.'TEST_BOUND_EQ'.$t_test ;

	  my $r = abc_bmc({  "INPUT_FILE" => "dump.v", 
			     "MAX_FRAMES" => $bmc_frameout,
			     "TIMEOUT"    => $bmc_timeout,
			     "EXP_NAME"   => $exp_name_bound,
			    });
	  print Dumper($r);
	  
	  if ($r->{INVALID}==1) {
	       $t_cex = $t_test;
	       $t_min = $t_test; 
	  } elsif (($r->{INVALID}==0) or ($r->{TIMEOUT}==1)) {
	       $t_max = $t_test - 1;
	  } else {
	       croak "ERROR: dont understand results\n".Dumper($r)."\n" ;
	  }
	  
	  $r->{LOG_FILE_NAME} = $exp_name_bound;
	  $r->{T_TEST} = $t_test;
	  $r->{T_CEX} = $t_cex;
	  push (@$R,$r);
	  print Dumper($R);
	  print sprint_bmc_table($R,$args);

	  writeFile($exp_name.".tex",sprint_bmc_table($R,$args));
	  $t_test = $t_min+1;  #for next step in linear search
     }

     appendFile("exp_results.txt",sprint_bmc_table($R,$args));
     print "find_t_cex finished, tcex = $t_cex\n";
     return $t_cex;
}





sub sprint_bmc_table {
     my $r    = shift;
     my $args = shift;

     my $t = "\n";
     $t .= "%\n";
     $t .= "%\n";
     $t .= "% ".$0."\n";
     $t .= "% timestamp   ".$args->{TIMESTAMP}."\n";
     $t .= "% exp_name    ".$args->{EXP_NAME}."\n";
     $t .= "% bmc_timeout ".$args->{BMC_TIMEOUT}."\n";
     $t .= "% bmc_frames  ".$args->{BMC_FRAMEOUT}."\n";
     $t .= "% build_args  ".$args->{BUILD_ARGS}."\n";
     $t .= "\\hline \n";
     $t .= '\multirow{'.(2+$#{$r}).'}{*}{{ '.$args->{EXP_NAME}.'}}'."\n"; 
     $t .= sprintf ("& %10s & %10s & %10s & %10s \\\\ \\cline{2-5}\n",
		    "Runtime(s)", 
		    "Frames",
		    "Found CEX", 
		    "T", 
		   );

     foreach my $i (sort {$a->{T_TEST} <=> $b->{T_TEST}} @{$r}) {
	  $t .= sprintf ("& %10.2f & %10i & %10i & %10i  \\\\  \n",			 
			 $i->{RUN_TIME},
			 $i->{FRAMES},
			 $i->{INVALID},
			 $i->{T_TEST},
			);
     }
     return $t;

}



sub readFileArray  {
     my ( $f ) = @_;
     open F, "< $f" or die "Can't open $f : $!";
     my @f = <F>;		close F;
     return @f;
}


sub writeFile {
     my ( $f, @data ) = @_;
     @data = () unless @data;
     open F, "> $f" or die "Can't open $f : $!";
     print F @data;
     close F;
}

sub appendFile {
     my ( $f, @data ) = @_;
     @data = () unless @data;
     open F, ">> $f" or die "Can't open $f : $!";
     print F @data;
     close F;
}


sub readFileScalar  {
     my ( $f ) = @_;
     open F, "< $f" or die "Can't open $f : $!";
     my @f = <F>;		close F;
     return "@f";
}

sub makeUnique {
     my @a = @_; 	
     my %saw;
     @a = grep(!$saw{$_}++, @a);   return @a;
}




sub Batch_Prove {
     my $experiments = shift;
     my $exp_name = shift;
     my $timeout = shift;

     # generate result for each experiment
     my $R = ();
     foreach my $i (0..$#{$experiments}) {
	  my $case_name = $exp_name."case$i";
	  my $build_args   = $experiments->[$i]->{BUILD_ARGS};
	  my $engine       = $experiments->[$i]->{ENGINE};

	  my $out = `./dump.out $build_args --dump dump.v`;
	  my $tproved = parse_proved_bound($out); #check what bound we actually proved
	  
	  my $r = "";
	  if ($engine eq "kind") {
 	       $r = abc_kind({ "INPUT_FILE" => "dump.v",
			       "MAX_FRAMES" => 500,
			       "TIMEOUT"    => $timeout,
			       "EXP_NAME"   => $case_name  })
	  } elsif ($engine eq "pdr") {
	       $r = abc_pdr({ "INPUT_FILE" => "dump.v",
			      "MAX_FRAMES" => 500,
			      "TIMEOUT"    => $timeout,
			      "EXP_NAME"   => $case_name  })
	  } else {
	       assert(0);
	  } 
	  if ($r->{TIMEOUT}) {
	       $r->{RUN_TIME} = $timeout;
	  }
	  $r->{ENGINE}          = "$engine";
	  $r->{BUILD_ARGS}      = "$build_args";
	  $r->{PROVED}          = ($r->{VALID} == 1) ? "Y" : "-";
	  $r->{NUM_ASSERTIONS}  = scalar parse_assertions("dump.v");
	  $r->{LOG_FILE_NAME}   = $case_name;
	  $r->{TPROVED}         = $tproved;

	  push (@$R,$r);
	  writeFile("current_results.txt",print_batch_table($R)); # just to check on progress
	  appendFile("current_results.txt",Dumper($R));
     }

     appendFile("exp_results.txt",print_batch_table($R,$timeout));
     writeFile($exp_name.".batch.tex",print_batch_table($R,$timeout)); # just to check on progress
     print "exiting normally\n";
     print Dumper($R);
     return $R;
}



# crudely turn dump.out args into something to use in
# plot legend or tex table. tweak by hand for final versions
# sub propdef_to_string {
#      my $d = shift;
#      my @symbols = ();

#      my @symbols = split(/(--)/,$d);
#      my @symbols2 = ();
#      for my $s (@symbols) {
# 	  if ($s =~ /^enable_tag.*/) {
# 	       $s =~ s/enable_tag//g;
# 	       $s =~ s/_/ /g;
# 	       push(@symbols2,$s);
# 	  }
# 	  elsif ($s =~ /^disable_tag.*/) {
# 	       $s =~ s/disable_tag\s+/NOT/g;
# 	       $s =~ s/_/ /g;
# 	       push(@symbols2,$s);
# 	  } elsif ($s =~ /^\s*--$/) { ;
# 	  } elsif ($s =~ /^\s*$/) { ;
# 	  } else { ; } #croak; 
#      }

# #     print Dumper(@symbols);
# #     print Dumper(@symbols2);
# #      for my $d (@symbols2) {
# # 	  $d =~ s/_/\\\_/g;
# # 	  $d =~ s/ /\\\ /g;
# # 	  print Dumper($d);
# #      }
# #      if (@symbols == 0) {
# # 	  push (@symbols, $d);
# #      }
#      my $s = " ".join(' AND ',@symbols2)." ";
# #     print Dumper($s);
#      return $s;
# }

# crudely turn dump.out args into something to use in
# plot legend or tex table. tweak by hand for final versions
sub propdef_to_tex_string {
     my $d = shift;
     return propdef_to_tex_or_gp_string($d, "tex");
}

sub propdef_to_gp_string {
     my $d = shift;
     return propdef_to_tex_or_gp_string($d, "gp");
}

sub propdef_to_tex_or_gp_string {
     my $d = shift;
     my $tex_or_gp = shift;

     $d = $d." ";
     my @symbols_tex = ();
     my @symbols_gp = ();

     if ($d =~ /\-\-enable_tag\s+phig_tl\s/) { 
	  push(@symbols_tex,'\Phi_{T_L}^G');
	  push(@symbols_gp ,'{/Symbol F}_{T_L}^G');
     }

     if ($d =~ /\-\-enable_tag\s+phig_x\s/) {
	  my $x = "T"; #sometimes the bound is not given in the args, but added later
	  if ($d =~ /.*t_bound\s+(\d+)\s/ ) {
	       $x = $1;
	  } 
	  push(@symbols_tex,'\Phi_{'.$x.'}^G' );
	  push(@symbols_gp ,'{/Symbol F}_{'.$x.'}^G');
     }

     if ($d =~ /\-\-enable_tag\s+psi\s/) {
	  push(@symbols_tex,'\Psi');
	  push(@symbols_gp ,'{/Symbol Y}');
     } 

     if ($d =~ /\-\-enable_tag\s+phil\s/) { 
	  push(@symbols_tex,'\Phi^L');
	  push(@symbols_gp ,'{/Symbol F}^L');
     }

     if ($d =~ /\-\-enable_tag\s+psil\s/) {
	  push(@symbols_tex,'\Psi^L');
	  push(@symbols_gp ,'{/Symbol Y}^L');
     } 


     my $encoding = "";
     if ($d =~ /\-\-t_style\s+binary_fixed\s/)        { $encoding = 'e1;'; } 
     if ($d =~ /\-\-t_style\s+binary_incrementing\s/) { $encoding = 'e2;'; } 
     if ($d =~ /\-\-t_style\s+unary_incrementing\s/)  { $encoding = 'e3;'; } 



     if ($tex_or_gp eq "tex") {
	  my $s = ' $'.join(' \wedge ',@symbols_tex).'$ ';
	  return $encoding.$s;

     } elsif ($tex_or_gp eq "gp") {
	  my $s = " ".join(' \\^ ',@symbols_gp)." ";
	  return $encoding.$s;
     }
     croak;
}


sub print_batch_table {
     my $r = shift;
     my $timeout = shift;
     my $timestamp = `date`;
     print Dumper($r);

     my $t = "\n% \n";
     $t .= "% ".prepend_lines_with("%",Dumper($r));
     $t .= "\n\n";
     $t .= "% ".$0."\n";
     $t .= "% ".$timestamp;
     $t .= "% ".Dumper($timeout);
     $t .= sprintf ("%12s & %6s & %6s & %6s & %42s \\\\ \\hline \\hline\n",
		    "Runtime (s)", "Frames", "Proved", "Engine","Property");

     foreach my $i (0..$#$r) {
	  $t .= sprintf ("%12.2f & %6s & %6s & %6s & %-70s \\\\ \\hline \n",
			 $r->[$i]->{RUN_TIME},
			 $r->[$i]->{FRAMES},
			 $r->[$i]->{PROVED},
			 $r->[$i]->{ENGINE},
			 propdef_to_tex_string($r->[$i]->{BUILD_ARGS}),
			);
     }
     return $t;
}

sub prepend_lines_with{
     my $prechar = shift;
     my $s = shift;
     my @lines = split(/\n/,$s);
     for my $line (@lines) {
	  $line = "$prechar $line";
     }
     return join("\n",@lines);
}

sub remove_underscore{
     my $s1 = shift;
     $s1 =~ s/_/ /g;
     return $s1;
}



1;
