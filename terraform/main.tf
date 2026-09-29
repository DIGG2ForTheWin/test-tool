provider "aws" {
  region = "eu-north-1"
}

data "aws_ami" "ubuntu" {
  most_recent = true

  filter {
    name = "name"
    values = ["ubuntu/images/hvm-ssd-gp3/ubuntu-noble-24.04-amd64-server-*"]
  }

  owners = ["099720109477"] # Canonical

}

resource "aws_security_group" "ssh" {
  name = "learn-terraform-ssh"

  ingress {
    from_port       = 22
    to_port         = 22
    protocol        = "tcp"
    # cidr_blocks     = ["13.48.4.200/30"]
    cidr_blocks     = ["0.0.0.0/0"]
  }
  egress {
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }
}

resource "aws_instance" "app_server" {
  ami           = data.aws_ami.ubuntu.id
  instance_type = "m7i-flex.large"
  key_name = aws_key_pair.me.key_name
  vpc_security_group_ids = [aws_security_group.ssh.id]

  tags = {
    Name = "learn-terraform"
  }
}

resource "aws_key_pair" "me" {
  key_name   = "learn-terraform-key"
  public_key = file("../Key.pub")
}

output "public_ip" {
  value = aws_instance.app_server.public_ip
}
