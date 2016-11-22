module Namespace
  def self.unshare(flag, options={})
    ret = do_unshare(flag)
    if ret < 0
      return ret
    end

    if path = options[:persist_to]
      each_flag(flag) do |f|
        persist_ns 0, f, sprintf(path, flag_to_string(f))
      end
    end
    return ret
  end

  def self.unshare!(flag, options={})
    ret = unshare(flag, options)
    raise("unshare failed") if ret < 0
    ret
  end
end
