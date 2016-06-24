desc 'create binary'
task 'mruby/bin/mrbtest' do
  sh "cd mruby && cp -fp ../.travis_build_config.rb build_config.rb && make all"
end

desc 'run test as root'
task :test => 'mruby/bin/mrbtest' do
  sh "cd mruby && sudo bin/mrbtest"
end

task :default => :test
