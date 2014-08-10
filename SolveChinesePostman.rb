#!/usr/bin/env ruby
# -*- coding: utf-8 -*-

require "set"

def readgraph(filename)
  result = []
  File.readlines(filename).each do |line|
    line.chomp!
    next if line.empty? || line[0] == "#"[0]
    weight, v1, v2 = line.split
    result << {:edges => Set[v1, v2], :weight => weight.to_i}
  end
  result
end

class Tee
  def initialize(io)
    @io = io
  end
  
  def puts(*args)
    @io.puts(*args)
    STDOUT.puts(*args)
  end
  
  def print(*args)
    @io.print(*args)
    STDOUT.print(*args)
  end
  
  def close
    @io.close
  end
end

def main
  if ARGV.empty?
    STDERR.puts "Usage(1): #{$0} [EdgesFile] [DivisionEdgesFile]"
    STDERR.puts "Usage(2): #{$0} [EdgesDirectory]"
    return
  end
  
  if FileTest.directory?(ARGV[0])
    ents = Dir.entries(ARGV[0])
    
    resfilename = ARGV[0] + "/result.edges"
    resfile = Tee.new(open(resfilename, "wb"))
    
    ents.grep(/\Asubgraph-(.+)\.edges\z/).each do |f|
      resfile.puts "# ========================================"
      resfile.puts "# Checking for the graph \"#{f}\""
      resfile.puts "# ========================================"
      
      division_graph_name = ARGV[0] + "/division" + f[8..-1]
      graph_name = ARGV[0] + "/" + f
      
      if FileTest.file?(division_graph_name)
        resfile.print `./SolveChinesePostman.exe \"#{graph_name}\" \"#{division_graph_name}\"`
      else
        resfile.print `./SolveChinesePostman.exe \"#{graph_name}\"`
      end #"#
    end
    
    resfile.close
    puts ""
    puts "The result is written to \"#{resfilename}\"."
    
    bridge_size = doubled_size = nil
    if FileTest.file?(ARGV[0] + "/bridges.edges")
      g = readgraph(ARGV[0] + "/bridges.edges")
      bridge_size = g.inject(0){ |i, j| i + j[:weight] }
      puts "@ Total distance of bridge edges = #{bridge_size}"
    end
    if FileTest.file?(resfilename)
      g = readgraph(resfilename)
      doubled_size = g.inject(0){ |i, j| i + j[:weight] }
      puts "@ Total distance of doubled edges other than bridge = #{doubled_size}"
    end
    puts "@ Total distance of doubled edges = #{bridge_size + doubled_size}"
    
  elsif FileTest.file?(ARGV[0])
    system "./SolveChinesePostman.exe", *ARGV
  end
end

main
