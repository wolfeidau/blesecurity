{
  'targets': [
   {
      'target_name': 'assert-security',
      'type': 'executable',
      'conditions': [
        ['OS=="linux"', {
          'sources': [
            'src/assert-security.c'
          ],
          'link_settings': {
            'libraries': [
              '-lbluetooth'
            ]
          }
        }]
      ]
    }
  ]
}
