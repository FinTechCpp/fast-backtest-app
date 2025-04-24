
# This hook runs at runtime to patch bokeh.sampledata
import sys
import types

def patch_bokeh_sampledata():
    # Create a more complete fake sampledata module
    fake_module = types.ModuleType('bokeh_sampledata')
    fake_module.__version__ = '0.1.0'  # Add a version attribute
    
    # Add any other required attributes
    fake_module.download = lambda: None
    fake_module.data_dir = "."
    
    # Register the fake module
    sys.modules['bokeh_sampledata'] = fake_module
    
    # Patch bokeh.sampledata if it exists
    try:
        # First register our fake module
        import bokeh
        if not hasattr(bokeh, 'sampledata'):
            # Create a submodule if it doesn't exist
            sampledata_module = types.ModuleType('bokeh.sampledata')
            sampledata_module.__path__ = []
            sampledata_module.download = lambda: None
            bokeh.sampledata = sampledata_module
    except ImportError:
        pass

# Execute patching
patch_bokeh_sampledata()
