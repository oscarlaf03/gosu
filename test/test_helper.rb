gem 'minitest'
require "minitest/autorun"
require "gosu" unless defined? Gosu

module TestHelper
  def media_path(fname='')
    File.join(__dir__, "media", fname)
  end
end

module InteractiveTests
  def interactive_cli(message)
    return false unless ENV["GOSU_TEST_INTERACTIVE"]
    
    STDOUT.puts message + 'Type (Y)es or (N)o and ENTER'
    yield if block_given?
    assert STDIN.gets.chomp =~ /[yY]/, "User answered 'No' to '#{message}'"
  end
end