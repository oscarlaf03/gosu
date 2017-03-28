# Encoding: UTF-8
require_relative 'test_helper'

# All Soundfiles are from http://www.bigsoundbank.com/ and "These files are free and completely royalty free for all uses."
class TestAudio < Minitest::Test
  include TestHelper
  include InteractiveTests
  
  def test_audio
    audio = Gosu::Audio.new(media_path('0614.ogg'))
    
    stream = audio.play
    assert stream.playing?
    
    interactive_cli("Do you hear a StarWars like Blaster sound?")
    
    stream.stop
    refute stream.playing?    
    
  end  
end