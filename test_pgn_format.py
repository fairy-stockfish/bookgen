#!/usr/bin/env python3
"""
Test script to verify PGN output format functionality
"""
import subprocess
import os
import tempfile

def run_stockfish_command(commands):
    """Run stockfish with given commands and return output"""
    cmd_string = '\n'.join(commands) + '\n'
    
    try:
        result = subprocess.run(
            ['./src/stockfish'],
            input=cmd_string,
            text=True,
            capture_output=True,
            timeout=30,
            cwd='/home/runner/work/bookgen/bookgen'
        )
        return result.stdout, result.stderr, result.returncode
    except subprocess.TimeoutExpired:
        return "", "Command timed out", 1

def test_epd_backward_compatibility():
    """Test that EPD format still works as before"""
    print("Testing EPD backward compatibility...")
    
    commands = [
        'setoption name BookFormat value epd',
        'setoption name EPDPath value test_epd_compat.epd',
        'position startpos',
        'generate 2 depth 1',
        'save',
        'quit'
    ]
    
    stdout, stderr, returncode = run_stockfish_command(commands)
    
    if returncode != 0:
        print(f"❌ EPD test failed with return code {returncode}")
        print(f"stderr: {stderr}")
        return False
    
    # Check if EPD file was created
    epd_file = '/home/runner/work/bookgen/bookgen/test_epd_compat.epd'
    if not os.path.exists(epd_file):
        print("❌ EPD file was not created")
        return False
    
    # Check EPD file content
    with open(epd_file, 'r') as f:
        content = f.read().strip()
        if not content:
            print("❌ EPD file is empty")
            return False
        if 'rnbqkbnr' not in content:  # Should contain FEN
            print("❌ EPD file doesn't contain expected FEN")
            return False
    
    print("✅ EPD backward compatibility test passed")
    return True

def test_pgn_output():
    """Test PGN output format"""
    print("Testing PGN output format...")
    
    commands = [
        'setoption name BookFormat value pgn',
        'setoption name BookPath value test_pgn_output',
        'position startpos',
        'generate 2 depth 1',
        'save',
        'quit'
    ]
    
    stdout, stderr, returncode = run_stockfish_command(commands)
    
    if returncode != 0:
        print(f"❌ PGN test failed with return code {returncode}")
        print(f"stderr: {stderr}")
        return False
    
    # Check if PGN file was created
    pgn_file = '/home/runner/work/bookgen/bookgen/test_pgn_output.pgn'
    if not os.path.exists(pgn_file):
        print("❌ PGN file was not created")
        return False
    
    # Check PGN file content
    with open(pgn_file, 'r') as f:
        content = f.read()
        
        # Check for PGN headers
        required_headers = ['[Event', '[Site', '[White', '[Black', '[Result', '[Variant']
        for header in required_headers:
            if header not in content:
                print(f"❌ PGN file missing required header: {header}")
                return False
        
        # Check for moves
        if 'e2e3' not in content or 'e7e6' not in content:
            print("❌ PGN file doesn't contain expected moves")
            return False
        
        # Check for proper move numbering
        if '1.' not in content:
            print("❌ PGN file doesn't contain proper move numbering")
            return False
    
    print("✅ PGN output format test passed")
    return True

def test_variant_support():
    """Test PGN output with different variants"""
    print("Testing PGN output with chess variants...")
    
    commands = [
        'setoption name UCI_Variant value chess',
        'setoption name BookFormat value pgn',
        'setoption name BookPath value test_variant',
        'position startpos',
        'generate 2 depth 1',
        'save',
        'quit'
    ]
    
    stdout, stderr, returncode = run_stockfish_command(commands)
    
    if returncode != 0:
        print(f"❌ Variant test failed with return code {returncode}")
        print(f"stderr: {stderr}")
        return False
    
    # Check PGN file content for variant
    pgn_file = '/home/runner/work/bookgen/bookgen/test_variant.pgn'
    if os.path.exists(pgn_file):
        with open(pgn_file, 'r') as f:
            content = f.read()
            if '[Variant "chess"]' not in content:
                print("❌ PGN file doesn't contain correct variant header")
                return False
    
    print("✅ Variant support test passed")
    return True

def cleanup_test_files():
    """Clean up test files"""
    test_files = [
        'test_epd_compat.epd',
        'test_pgn_output.pgn',
        'test_variant.pgn'
    ]
    
    for file in test_files:
        filepath = f'/home/runner/work/bookgen/bookgen/{file}'
        if os.path.exists(filepath):
            os.remove(filepath)

def main():
    """Run all tests"""
    print("Running PGN format tests...\n")
    
    tests = [
        test_epd_backward_compatibility,
        test_pgn_output,
        test_variant_support
    ]
    
    passed = 0
    total = len(tests)
    
    for test in tests:
        if test():
            passed += 1
        print()
    
    print(f"Test Results: {passed}/{total} tests passed")
    
    if passed == total:
        print("🎉 All tests passed! PGN output format is working correctly.")
    else:
        print("❌ Some tests failed. Please check the implementation.")
    
    cleanup_test_files()
    return passed == total

if __name__ == '__main__':
    main()