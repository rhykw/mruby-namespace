module Namespace
  def self.flag_to_string(flag)
    base = {
      CLONE_NEWNS => "mount",
      CLONE_NEWUTS => "uts",
      CLONE_NEWIPC => "ipc",
      CLONE_NEWUSER => "user",
      CLONE_NEWNET => "net",
    }
    if(CLONE_NEWCGROUP rescue false)
      base[CLONE_NEWCGROUP] = "cgroup"
    end

    return base[flag]
  end

  def self.each_flag(flag)
    flags_base = [
      CLONE_NEWNS,
      CLONE_NEWUTS,
      CLONE_NEWIPC,
      CLONE_NEWUSER,
      CLONE_NEWNET,
    ]
    if(CLONE_NEWCGROUP rescue false)
      flags_base << CLONE_NEWCGROUP
    end

    flags_active = flags_base.map{|f| flag & f }.delete_if{|f| f.zero? }
    if block_given?
      flags_active.each {|f| yield f }
    else
      flags_active.to_enum
    end
  end

  def persist_all_ns(pid, flag, options={})
    path = options[:to] || options[:format]
    raise(ArgumentError, "option to: or format: required") unless path
    each_flag(flag) do |f|
      persist_ns pid, f, sprintf(path, flag_to_string(f))
    end
  end
end
