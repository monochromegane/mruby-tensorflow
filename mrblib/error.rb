unless Object.const_defined?("NotFoundError")
  class NotFoundError < StandardError; end
end

unless Object.const_defined?("SessionError")
  class SessionError < StandardError; end
end
