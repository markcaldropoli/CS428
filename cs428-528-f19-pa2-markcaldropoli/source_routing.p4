/*
Copyright 2013-present Barefoot Networks, Inc. 

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#include <core.p4>
#include <v1model.p4>

// TODO: define headers & header instances
header easyRoute_head_t {
    bit<64> preamble;
    bit<32> num_valid;
}

header easyRoute_port_t {
    bit<8> port;
}

struct headers {
    easyRoute_head_t easyRoute_head;
    easyRoute_port_t easyRoute_port;
}

struct metadata {
    /* empty */
}

parser MyParser(packet_in packet,
	out headers hdr,
	inout metadata meta,
	inout standard_metadata_t standard_metadata) {
    // TODO: define parser states
    state start {
        transition parse_head;
    }

    state parse_head {
        packet.extract(hdr.easyRoute_head);
        transition select(hdr.easyRoute_head.num_valid) {
            0: accept;
            default: parse_port;
        }
    }

    state parse_port {
        packet.extract(hdr.easyRoute_port);
        transition accept;
    }
}

control MyVerifyChecksum(inout headers hdr, inout metadata meta) {
    // leave empty
    apply {  }
}

control MyIngress (inout headers hdr,
		   inout metadata meta,
		   inout standard_metadata_t standard_metadata){

    action drop() {
	    mark_to_drop();
    }
    // TODO: route action to update packet headers & metadata
    action route_packet() {
        standard_metadata.egress_spec = (bit<9>)hdr.easyRoute_port.port;
        hdr.easyRoute_head.num_valid = hdr.easyRoute_head.num_valid - 1;
        hdr.easyRoute_port.setInvalid();
    }

    apply {
        if(hdr.easyRoute_port.isValid()) {
            route_packet();
        } else {
            drop();
        }
    }
}

control MyEgress(inout headers hdr,
                 inout metadata meta,
                 inout standard_metadata_t standard_metadata) {
    // leave empty
    apply { }
}

control MyComputeChecksum(inout headers hdr, inout metadata meta) {
    // leave empty
    apply {  }
}

control MyDeparser(packet_out packet, in headers hdr) {
    // TODO: implement deparser
    apply {
        packet.emit(hdr.easyRoute_head);
        packet.emit(hdr.easyRoute_port);
    }
}

V1Switch(
MyParser(),
MyVerifyChecksum(),
MyIngress(),
MyEgress(),
MyComputeChecksum(),
MyDeparser()
) main;
