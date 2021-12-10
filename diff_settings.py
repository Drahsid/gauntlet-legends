#!/usr/bin/env python3

def apply(config, args):
    config['mapfile'] = f'build/gauntletlegends.map'
    config['myimg'] = f'build/gauntletlegends.z64'
    config['baseimg'] = f'baserom.z64'
    config['source_directories'] = ['src', 'include', 'asm']
