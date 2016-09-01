MRuby::Gem::Specification.new('mruby-linux-namespace') do |spec|
  spec.license = 'MIT'
  spec.authors = 'Russel Hunter Yukawa/Uchio Kondo'

  spec.add_test_dependency 'mruby-dir'
  spec.add_test_dependency 'mruby-process'
end
