locals {
  config = yamldecode(file("${path.module}/../tests.yaml"))
  machines = { for m in local.config.infrastructure.machines : m.name => m }
  user = local.config.infrastructure.user
}

provider "aws" {
  region = local.config.infrastructure.region
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
  name = "${local.user}-terraform-ssh"

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
  
  ingress {
    from_port = 8080   # cAdvisor
    to_port   = 8080
    protocol  = "tcp"
    self      = true
  }
  ingress {
    from_port = 9100   # node-exporter
    to_port   = 9100
    protocol  = "tcp"
    self      = true
  }
  ingress {
    from_port = 8081   # Domibus blue (AS4 messages between machines)
    to_port   = 8082   # Domibus red
    protocol  = "tcp"
    self      = true
  }

}

resource "aws_instance" "app_server" {
  for_each = local.machines

  ami           = data.aws_ami.ubuntu.id
  instance_type = each.value.instance_type
  key_name = aws_key_pair.me.key_name
  vpc_security_group_ids = [aws_security_group.ssh.id]

  root_block_device {
    volume_size = each.value.storage_gb
  }

  tags = {
    Name = "${local.user}-${each.value.name}"
  }
}

resource "aws_key_pair" "me" {
  key_name   = "${local.user}-terraform-key"
  public_key = file("../Key.pub")
}

output "public_ips" {
  value = { for name, server in aws_instance.app_server : name => server.public_ip }
}

output "private_ips" {
  value = { for name, s in aws_instance.app_server : name => s.private_ip }
}
