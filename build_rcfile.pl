#!/usr/bin/perl

# Get kernel version (on >= 2.6 alsa is preferred)
$_ = `uname -r`;
@fields = split /\./;
$alsa_preferred = (($fields[0] > 2) || ($fields[0] == 2 && $fields[1] >= 6));

# Choose output plugin
$output_plugin = "oss";
$_ = $ARGV[0];
$output_plugin = "alsa" if (($alsa_preferred && /\balsa\b/) || !/\boss\b/);

# Write configuration
print "lib-dir = ../plugins\n" if ($ARGV[1] =~ /^local$/);
print "output-plugin = $output_plugin\n";

