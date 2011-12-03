$:.unshift(File.dirname(__FILE__) + "/../lib/")

require 'test/unit'
require "raspell"

class TestSpell < Test::Unit::TestCase

  def setup
    @aspell = Aspell.new
    @text = ["Hiere is somthing wrong on the planett. And it was not the Apollo."]
  end

  def test_correct_lines
    assert_equal(["<wrong word> is <wrong word> wrong on the <wrong word>. And it was not the Apollo."],
                 @aspell.correct_lines(@text) { |word| "<wrong word>" })
  end

  def test_list_mispelled
    misspelled = @aspell.list_misspelled(@text).sort
    assert_equal(3, misspelled.length)
  end

  def test_suggest
    suggestions =  @aspell.suggest("spel")
    assert_equal [],
      ["spell", "spiel", "spew", "Opel", "spec", "sped"] - suggestions
  end

  def test_check
    assert_equal(false, @aspell.check("spel"))
    assert(@aspell.check("spell"))
  end

  def test_argument_checking
    assert_raises TypeError do
      @aspell.list_misspelled "Not an array"
    end
    assert_raises TypeError do
      @aspell.list_misspelled [123]
    end
  end
end


