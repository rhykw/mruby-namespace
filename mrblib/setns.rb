module Namespace
  def self.setns(flag, options)
    fd = options[:fd]
    pid = options[:pid]

    if fd
      setns_by_fd(fd, flag)
    elsif pid
      setns_by_pid(pid, flag)
    else
      raise ArgumentError, "Options :fd or :pid must be specified"
    end
  end
end
